#define TB_IMPL
#define TB_LIB_OPTS
#define TB_OPT_TRUECOLOR

#include "termbox.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <float.h>
#include <math.h>

#define MIN_NBALLS 5
#define MAX_NBALLS 20
#define DIGIT_W 3
#define DIGIT_H 5
#define SCALE 8
#define BG_COLOR 0x111111

static const bool number[][15] =
{
    {1,1,1,1,0,1,1,0,1,1,0,1,1,1,1},
    {0,0,1,0,0,1,0,0,1,0,0,1,0,0,1},
    {1,1,1,0,0,1,1,1,1,1,0,0,1,1,1},
    {1,1,1,0,0,1,1,1,1,0,0,1,1,1,1},
    {1,0,1,1,0,1,1,1,1,0,0,1,0,0,1},
    {1,1,1,1,0,0,1,1,1,0,0,1,1,1,1},
    {1,1,1,1,0,0,1,1,1,1,0,1,1,1,1},
    {1,1,1,0,0,1,0,0,1,0,0,1,0,0,1},
    {1,1,1,1,0,1,1,1,1,1,0,1,1,1,1},
    {1,1,1,1,0,1,1,1,1,0,0,1,1,1,1},
};

typedef struct {
  float x;
  float y;
  float vx;
  float vy;
  float heat;
  int restTicks;
} Ball;

static unsigned char clockMask[512][512];

static unsigned char lavaMask[512][512];

uintattr_t pallete[11];
int baseColor[3];
int baseColor2[3];
short gradient1=0;
short gradient2=0;

static char *custom = NULL;
static char *custom2 = NULL;
static short color = TB_WHITE;
static short color2 = TB_WHITE;
static short party = 0;
static int nballs = 10;
static short speedMult = 5;
static short rim = 1;
static short contained = 0;
static short gravityMode = 0;
static float gravityStrength = 0.12f;
static float buoyancyStrength = 0.22f;
static float heatGain = 0.035f;
static float heatLoss = 0.0045f;
static float bounceDamp = 0.85f;
static float radiusIn = 110;
static float radius;
static int margin;
static float sumConst;
static float sumConst2;
static int maxX, maxY;
static int speed;
static Ball balls[MAX_NBALLS] = {0};
static struct tb_event event = {0};
static short colors[]={TB_WHITE, TB_RED, TB_YELLOW, TB_BLUE, TB_GREEN, TB_MAGENTA, TB_CYAN, TB_BLACK };

void init_params();
void event_handler();
int parse_options(int argc, char *argv[]);
void print_help();
short next_color(short current);
void fix_rim_color();
void set_random_colors(short level);

static void clear_clock_mask(void)
{
    memset(clockMask,0,sizeof(clockMask));
}

static void draw_digit(int digit,int ox,int oy)
{
    for(int y=0;y<5;y++)
    {
        for(int x=0;x<3;x++)
        {
            if(number[digit][y*3+x])
            {
                for(int yy=0;yy<SCALE;yy++)
                {
                    for(int xx=0;xx<SCALE;xx++)
                    {
                        int px=ox+x*SCALE+xx;
                        int py=oy+y*SCALE+yy;

                        if(px>=0 && px<maxX &&
                           py>=0 && py<maxY)
                        {
                            clockMask[py][px]=1;
                        }
                    }
                }
            }
        }
    }
}

static void draw_colon(int ox,int oy)
{
    for(int yy=0;yy<SCALE;yy++)
    {
        clockMask[oy+SCALE+yy][ox]=1;
        clockMask[oy+SCALE+yy][ox+1]=1;

        clockMask[oy+SCALE*3+yy][ox]=1;
        clockMask[oy+SCALE*3+yy][ox+1]=1;
    }
}

static void draw_clock(void)
{
    clear_clock_mask();

    time_t now=time(NULL);
    struct tm *tm=localtime(&now);

    int h=tm->tm_hour;
    int m=tm->tm_min;

    int digits[4]=
    {
        h/10,
        h%10,
        m/10,
        m%10
    };

    int width=4*(DIGIT_W*SCALE)+2*(SCALE+2);
    int startx=(maxX-width)/2;
    int starty=(maxY-5*SCALE)/2;

    draw_digit(digits[0],startx,starty);

    startx+=DIGIT_W*SCALE+2;

    draw_digit(digits[1],startx,starty);

    startx+=DIGIT_W*SCALE+2;

    if ((tm->tm_sec % 2) == 0)
      draw_colon(startx, starty);

    startx+=SCALE+2;

    draw_digit(digits[2],startx,starty);

    startx+=DIGIT_W*SCALE+2;

    draw_digit(digits[3],startx,starty);
}

void set_pallete();
uintattr_t get_color(float val);
void set_pallete2();

int set_color(short *var, int *baseColor, char *optarg, short useGradient) {
  if (useGradient) {
    // Parse hex color
    if (sscanf(optarg, "%02x%02x%02x", &baseColor[0], &baseColor[1], &baseColor[2]) == 3) {
      return 1;
    } else {
      printf("Invalid hex color format. Use format: RRGGBB\n");
      return 0;
    }
  } else {
    // Parse named color
    if (strcmp(optarg, "red") == 0) {
      *var = TB_RED;
    } else if (strcmp(optarg, "yellow") == 0) {
      *var = TB_YELLOW;
    } else if (strcmp(optarg, "blue") == 0) {
      *var = TB_BLUE;
    } else if (strcmp(optarg, "green") == 0) {
      *var = TB_GREEN;
    } else if (strcmp(optarg, "magenta") == 0) {
      *var = TB_MAGENTA;
    } else if (strcmp(optarg, "cyan") == 0) {
      *var = TB_CYAN;
    } else if (strcmp(optarg, "black") == 0) {
      *var = TB_BLACK;
    } else if (strcmp(optarg, "white") == 0) {
      *var = TB_WHITE;
    } else {
      printf("Unknown color: %s\n", optarg);
      return 0;
    }
    return 1;
  }
}

int main(int argc, char *argv[]) {

  if (!parse_options(argc, argv))
    return 0;

  time_t t;
  // Ball *balls = malloc(sizeof(Ball) * nballs);

  srand((unsigned)time(&t));

  tb_init();

  tb_hide_cursor();

  init_params();


  while (1) {

    draw_clock();

    memset(lavaMask, 0, sizeof(lavaMask));

    // move balls
    for (int i = 0; i < nballs; i++) {

      if (gravityMode) {
        float minY = (float)margin;
        float maxYf = (float)(maxY - margin - 1);
        float topZone = minY + 6.0f;
        float bottomZone = maxYf - 6.0f;

        if (balls[i].restTicks > 0) {
          balls[i].restTicks--;
          balls[i].vy = 0.0f;
          balls[i].heat -= heatLoss * 2.0f;
        } else {
          if (balls[i].y > bottomZone) {
            balls[i].heat += heatGain;
          } else {
            balls[i].heat -= heatLoss;
          }

          if (balls[i].y < topZone) {
            balls[i].heat -= heatLoss * 1.5f;
          }

          balls[i].vy += gravityStrength - buoyancyStrength * balls[i].heat;
        }

        if (balls[i].heat < 0.0f)
          balls[i].heat = 0.0f;
        if (balls[i].heat > 1.0f)
          balls[i].heat = 1.0f;

        // Drag helps create the "hang time" at the top.
        balls[i].vx *= 0.995f;
        balls[i].vy *= 0.997f;
      }

      float nextX = balls[i].x + balls[i].vx;
      float nextY = balls[i].y + balls[i].vy;
      float minX = (float)margin;
      float maxXf = (float)(maxX - margin - 1);
      float minY = (float)margin;
      float maxYf = (float)(maxY - margin - 1);

      if (balls[i].vy > 2.5f)
        balls[i].vy = 2.5f;
      if (balls[i].vy < -2.5f)
        balls[i].vy = -2.5f;

      if (nextX > maxXf) {
        balls[i].x = maxXf;
        balls[i].vx = -balls[i].vx;
        if (gravityMode) {
          balls[i].vx *= bounceDamp;
        }
      } else if (nextX < minX) {
        balls[i].x = minX;
        balls[i].vx = -balls[i].vx;
        if (gravityMode) {
          balls[i].vx *= bounceDamp;
        }
      } else {
        balls[i].x = nextX;
      }

      if (nextY > maxYf) {
        balls[i].y = maxYf;
        // With gravity mode enabled, blobs should "sit" at the bottom and
        // get heated (buoyant) rather than bounce.
        if (gravityMode) {
          // Only enter a "rest" period on a real impact; otherwise keep
          // sticking to the floor and allow heat to accumulate.
          if (balls[i].restTicks == 0 && balls[i].vy > 0.4f) {
            balls[i].heat = 0.0f;
            balls[i].restTicks = 15 + (rand() % 45);
          }
          balls[i].vy = 0.0f;
        } else {
          balls[i].vy = -fabsf(balls[i].vy);
        }
      } else if (nextY < minY) {
        balls[i].y = minY;
        // With gravity mode enabled, blobs can linger at the top until they cool.
        if (gravityMode) {
          balls[i].vy = 0.0f;
        } else {
          balls[i].vy = fabsf(balls[i].vy);
        }
      } else {
        balls[i].y = nextY;
      }

      // Keep some motion even with damping.
      if (fabsf(balls[i].vx) < 0.15f) {
        balls[i].vx = (balls[i].vx < 0 ? -0.15f : 0.15f);
      }
    }

    // render
    for (int i = 0; i < maxX; i++) {
      for (int j = 0; j < maxY / 2; j++) {
        // calculate the two halfs of the block at the same time
        float sum[2] = {0};

        int topClock = clockMask[j * 2][i];
        int bottomClock = 0;

        if (j * 2 + 1 < maxY)
          bottomClock = clockMask[j * 2 + 1][i];

        for (int j2 = 0; j2 < (!custom ? 2 : 1); j2++) {

          for (int k = 0; k < nballs; k++) {
            int y = j * 2 + j2;
            float dx = (float)i - balls[k].x;
            float dy = (float)y - balls[k].y;
            float dist_squared = dx * dx + dy * dy;
            if (dist_squared == 0) {
              sum[j2] += FLT_MAX; 
            } else {
              sum[j2] += (radius * radius) / dist_squared;
            }
          }
        }

        if (sum[0] > sumConst)
          lavaMask[j * 2][i] = 1;

        if (sum[1] > sumConst && j * 2 + 1 < maxY)
          lavaMask[j * 2 + 1][i] = 1;

        if (!custom) {
          if(gradient1){
            if (sum[0] > sumConst) {
              if (sum[1] > sumConst) {
                tb_printf(i, j, get_color( sum[0]), get_color(sum[1]), "▀");
              } else {
                tb_printf(i, j, get_color( sum[0]), TB_TRUECOLOR_DEFAULT, "▀");
              }
            } else if (sum[1] > sumConst) {
              tb_printf(i, j,get_color( sum[1]),TB_TRUECOLOR_DEFAULT, "▄");
            }

          }else{
            if (sum[0] > sumConst) {
              if (sum[1] > sumConst) {
                tb_printf(i, j, color2, 0, "█");
              } else {
                tb_printf(i, j, color2, 0, "▀");
              }
            } else if (sum[1] > sumConst) {
              tb_printf(i, j, color2, 0, "▄");
            }

            if (rim) {
              if (sum[0] > sumConst2) {
                if (sum[1] > sumConst2) {
                  tb_printf(i, j, color, 0, "█");
                } else {
                  tb_printf(i, j, color2, color, "▄");
                }
              } else if (sum[1] > sumConst2) {
                tb_printf(i, j, color2, color, "▀");
              }
            }
          }
        } else {
          if (sum[0] > sumConst) {
            tb_printf(i, j, color2, 0, custom2);
          }

          if (rim) {
            if (sum[0] > sumConst2) {
              tb_printf(i, j, color, 0, custom);
            }
          }
        }
      }
    }
    if (party>0){
      set_random_colors(party);
    }

    for (int y = 0; y < maxY; y += 2)
    {
      for (int x = 0; x < maxX; x++)
      {
        int top = clockMask[y][x];
        int bottom = (y + 1 < maxY) ? clockMask[y + 1][x] : 0;

        if (!top && !bottom)
            continue;

        int lavaTop = lavaMask[y][x];
        int lavaBottom = (y + 1 < maxY) ? lavaMask[y + 1][x] : 0;

        uintattr_t fg = color2;
        uintattr_t bg = BG_COLOR;

        if ((top && lavaTop) || (bottom && lavaBottom))
        {
            fg = BG_COLOR;
            bg = color2;
        }

        tb_printf(x, y / 2, fg, bg, "█");
      }
    }

    tb_present();
    usleep(speed);
    tb_clear();

    tb_peek_event(&event, 10);

    event_handler();
  }

  tb_shutdown();

  // free(balls);
}

void event_handler() {
  if (event.type == TB_EVENT_RESIZE) {
    do
      tb_peek_event(&event, 10);
    while (event.type == TB_EVENT_RESIZE);

    init_params();
  } else if (event.type == TB_EVENT_KEY) {

    if (event.key == TB_KEY_CTRL_C || event.key == TB_KEY_ESC) {
      tb_shutdown();
      exit(0);
    }

    switch (event.ch) {
    case '-':
    case '_':
      if (speedMult < 10) {
        speedMult++;
        speed = (((1 / (float)(maxX + maxY)) * 1000000) + 10000) * speedMult;
      }
      break;
    case '+':
    case '=':
      if (speedMult > 1) {
        speedMult--;
        speed = (((1 / (float)(maxX + maxY)) * 1000000) + 10000) * speedMult;
      }
      break;
    case 'm':
    case 'M':
      if (nballs + 1 <= MAX_NBALLS) {
        nballs++;
      }
      break;
    case 'l':
    case 'L':
      if (nballs - 1 >= MIN_NBALLS) {
        nballs--;
      }
      break;
    case 'i':
      if (radiusIn + 5 <= 150) {
        radiusIn += 5;
        radius = (radiusIn * radiusIn + (float)(maxX * maxY)) / 15000;
        margin = contained ? radius * 10 : 0;
      }
      break;
    case 'd':
      if (radiusIn - 5 >= 100) {
        radiusIn -= 5;
        radius = (radiusIn * radiusIn + (float)(maxX * maxY)) / 15000;
        margin = contained ? radius * 10 : 0;
      }
      break;
    case 'I':

      if (color != TB_WHITE || custom || gradient1 )
        if (rim + 1 <= 5) {
          rim++;
          sumConst2 = sumConst * (1 + (float)(0.25 * rim));
        }
      break;
    case 'D':

      if (color != TB_WHITE || custom || gradient1)
        if (rim - 1 >= 0) {
          rim--;
          sumConst2 = sumConst * (1 + (float)(0.25 * rim));
        }
      break;
    case 'c':
      color = next_color(color);
      fix_rim_color();
      break;
    case 'k':
      color2 = next_color(color2);
      fix_rim_color();
      break;
    case 'p':
      party = (party+1)%4;
      break;
    case 'q':
    case 'Q':
      tb_shutdown();
      exit(0);
      break;
    }
  }
}

void init_params() {

  maxX = tb_width();
  maxY = tb_height() * 2;
  speedMult = 11 - speedMult;
  speed = (((1 / (float)(maxX + maxY)) * 1000000) + 10000) * speedMult;
  radius = (radiusIn * radiusIn + (float)(maxX * maxY)) / 15000;

  margin = contained ? radius * 10 : 0;

  sumConst = 0.0225;
  sumConst2 = sumConst * (1 + (float)(0.25 * rim));

  custom2 = custom;

  if (color2 == TB_WHITE || !rim)
    color2 = color | TB_BOLD;

  if (custom && strlen(custom) > 1 && rim) {
    custom2 = custom + 1;
  }

  for (int i = 0; i < MAX_NBALLS; i++) {
    balls[i].x = (float)(rand() % (maxX - 2 * margin) + margin);
    balls[i].y = (float)(rand() % (maxY - 2 * margin) + margin);
    balls[i].vx = (rand() % 2 == 0) ? -1.0f : 1.0f;
    balls[i].vy = (rand() % 2 == 0) ? -1.0f : 1.0f;
    balls[i].heat = (float)(rand() % 101) / 100.0f;
    balls[i].restTicks = rand() % 30;
  }
  if(gradient1){
    tb_set_output_mode(TB_OUTPUT_TRUECOLOR);
    tb_set_clear_attrs(TB_TRUECOLOR_DEFAULT, TB_TRUECOLOR_DEFAULT);
    if(gradient2) set_pallete2();
    else set_pallete();
  }
}

short next_color(short current){
  for(int i = 0; i<8; i++){
    if((current == colors[i])||(current == (colors[i] | TB_BOLD ))){
      return colors[(i+1)%8];
    }
  }
  return colors[0];
}

void fix_rim_color(){
  if(color2 == color){
    color2 = color2 | TB_BOLD;
  }
}

void set_random_colors( short level){
  if(level==1 || level==3) color = colors[ rand() % 7];
  if(level==2 || level==3) color2 = colors[ rand() % 7];
  fix_rim_color();
}

int parse_options(int argc, char *argv[]) {
  if (argc == 1)
    return 1;
  
  int c;
  // First pass to check for gradient mode
  optind = 1;  // Reset getopt
  while ((c = getopt(argc, argv, ":c:k:s:r:R:b:F:Cp:hgG")) != -1) {
    if (c == 'g') {
      gradient1 = 1;
      if (!tb_has_truecolor()) {
        fprintf(stderr, "The terminal does not support truecolor\n");
        return 0;
      }
      break;
    }
  }

  // Reset getopt for second pass
  optind = 1;
  while ((c = getopt(argc, argv, ":c:k:s:r:R:b:F:Cp:hgG")) != -1) {
    switch (c) {
    case 'c':
      if (!set_color(&color, baseColor, optarg, gradient1))
        return 0;
      break;
    case 'k':
      if (!set_color(&color2, baseColor2, optarg, gradient1))
        return 0;
      gradient2 = gradient1;  // If we're using gradient, enable the second gradient too
      break;
    case 's':
      speedMult = atoi(optarg);
      if (speedMult > 10 || speedMult <= 0) {
        printf("Invalid speed, only values between 1 and 10 are allowed\n");
        return 0;
      }
      break;
    case 'R':
      rim = atoi(optarg);
      if (rim > 5 || rim < 1) {
        printf("Invalid rim, only values between 1 and 5 are allowed\n");
        return 0;
      }
      break;
    case 'r':
      radiusIn = 100 + atoi(optarg) * 5;
      if (radiusIn > 150 || radiusIn < 100) {
        printf("Invalid radius, only values between 1 and 10 are allowed\n");
        return 0;
      }
      break;
    case 'b':
      nballs = atoi(optarg);
      if (nballs > MAX_NBALLS || nballs < MIN_NBALLS) {
        printf("Invalid number of metaballs, only values between %i and %i are "
               "allowed\n",
               MIN_NBALLS, MAX_NBALLS);
        return 0;
      }
      break;
    case 'F':
      custom = optarg;
      break;
    case 'C':
      contained = 1;
      break;
    case 'G':
      gravityMode = 1;
      break;
    case 'p':
      party = atoi(optarg);
      if (party < 0 || party > 3) {
        printf("Invalid party mode, only values between 1 and 3 are allowed\n");
        return 0;
      }
      break;
    case 'h':
      print_help();
      return 0;
      break;
    case 'g':
      // Already handled in first pass
      break;
    case ':':
      fprintf(stderr, "Option -%c requires an operand\n", optopt);
      return 0;
      break;
    case '?':
      fprintf(stderr, "Unrecognized option: -%c\n", optopt);
      return 0;
    }
  }
  return 1;
}

void print_help() {
  printf(
      "Usage: lavatty-clock [OPTIONS]\n"
      "OPTIONS:\n"
      "  -g                  Enable gradient mode with truecolor support.\n"
      "                      Changes how -c and -k options work.\n"
  "  -G                  Enable gravity and buoyancy movement (balls heat up at the bottom, rise, cool at the top, then fall).\n"
      "  -c <COLOR>          Set color. In normal mode, available colors are: red, blue, yellow, "
      "green, cyan, magenta, white, and black.\n"
      "                      In gradient mode (-g), use hex format: RRGGBB (e.g., FF0000 for red).\n"
      "  -k <COLOR>          Set the rim color. Same format options as -c.\n"
      "                      In gradient mode, this sets the second color for the gradient.\n"
      "  -s <SPEED>          Set the speed, from 1 to 10. (default 5)\n"
      "  -r <RADIUS>         Set the radius of the metaballs, from 1 to 10. "
      "(default: 5)\n"
      "  -R <RIM>            Set a rim for each metaball, sizes from 1 to 5."
      "(default: none)\n"
      "                      This option does not work with the default "
      "color\n"
      "                      If you use Kitty or Alacritty you must use it "
      "with the -k option to see the rim.\n"
      "  -b <NBALLS>         Set the number of metaballs in the simulation, "
      "from %i to %i. (default: 10)\n"
      "  -F <CHARS>          Allows for a custom set of chars to be used\n"
      "                      Only ascii symbols are supported for now, "
      "wide/unicode chars may appear broken.\n"
      "  -C                  Retain the entire lava inside the terminal.\n"
      "                      It may not work well with a lot of balls or with"
      " a bigger radius than the default one.\n"
      "  -p <MODE>           PARTY!! THREE MODES AVAILABLE (p1, p2 and p3).\n"
      "  -h                  Print help.\n"
      "RUNTIME CONTROLS:\n"
      "  i                   Increase radius of the metaballs.\n"
      "  d                   Decrease radius of the metaballs.\n"
      "  shift i             Increase rim of the metaballs.\n"
      "  shift d             Decrease rim of the metaballs.\n"
      "  m                   Increase the number of metaballs.\n"
      "  l                   Decrease the number metaballs.\n"
      "  c                   Change the color of the metaballs.\n"
      "  k                   Change the rim color of the metaballs.\n"
      "  +                   Increase speed.\n"
      "  -                   Decrease speed.\n"
      "  p                   TURN ON THE PARTY AND CYCLE THROUGH THE PARTY MODES "
      "(it can also turns off the party).\n"
      "(Tip: Zoom out in your terminal before running the program to get a "
      "better resolution of the lava).\n"
      "EXAMPLES:\n"
      "  lavatty-clock -c green -k red        Use named colors in normal mode\n"
      "  lavatty-clock -g -c 00FF00 -k FF0000 Use hex colors in gradient mode\n"
      "  lavatty-clock -G                     Start with gravity mode enabled\n",
      MIN_NBALLS, MAX_NBALLS);
}

void set_pallete(){
  int avgColor= (baseColor[0] + baseColor[1] + baseColor[2])/3;
  int blackfactor[5]; 
  int whitefactor[5]; 
  for(int i=1 ;i<6;i++){
    blackfactor[i-1]=(6-i)*avgColor/5;
    whitefactor[i-1]=i*(255-avgColor)/5;
  }

  for(int i=0 ;i<5;i++){
    int r, g, b;
    float factor = (1 - ((float)blackfactor[i]/(avgColor)));
    r = baseColor[0]*factor;
    g = baseColor[1]*factor;
    b = baseColor[2]*factor;
    pallete[i]= (r << 16) | (g << 8) | b;
    r = baseColor[0] + (255-baseColor[0])*whitefactor[i]/(255-avgColor);
    g = baseColor[1] + (255-baseColor[1])*whitefactor[i]/(255-avgColor);
    b = baseColor[2] + (255-baseColor[2])*whitefactor[i]/(255-avgColor);
    pallete[i+6] = (r << 16) | (g << 8) | b;

  }
  pallete[5]= (baseColor[0] << 16) | (baseColor[1] << 8) | baseColor[2];
}

void set_pallete2() {
  for (int i = 0; i < 11; i++) {
    float t = (float)i / (11 - 1); 
    
    int r = (1 - t) * baseColor[0] + t * baseColor2[0];
    int g = (1 - t) * baseColor[1] + t * baseColor2[1];
    int b = (1 - t) * baseColor[2] + t * baseColor2[2];

    pallete[i] = (r << 16) | (g << 8) | b;
  }
}

uintattr_t get_color(float val) {
    val = (val - sumConst)/(0.001*(rim+1)); 
    
    if (val > 10) {
        return pallete[10];       
    } else if (val < 0) {
        return pallete[0];        
    }

    return pallete[(int)val];     
}

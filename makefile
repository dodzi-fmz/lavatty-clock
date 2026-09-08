PREFIX = /usr/local

lavat: lavatty-clock.c
	$(CC) lavatty-clock.c -o lavatty-clock

.PHONY: clean
clean:
	$(RM) lavatty-clock

.PHONY: install
install: lavatty-clock
	mkdir -p $(PREFIX)/bin
	install lavatty-clock $(PREFIX)/bin/lavatty-clock

.PHONY: uninstall
uninstall:
	$(RM) $(PREFIX)/bin/lavatty-clock


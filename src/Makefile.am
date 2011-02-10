NAME=libvirt-php
CC=cc
CP=cp
RM=rm
ECHO=echo
PHPINC=`php-config --includes`
PHPEDIR=`php-config --extension-dir`
XMLCFLAGS=`pkg-config libxml-2.0 --cflags`
XMLLIBS=`pkg-config libxml-2.0 --libs`

EXTRA_DIST = libvirt.c php_libvirt.h

all: build clean-temp

build:
	$(CC) -fpic -DCOMPILE_DL_LIBVIRT=1 $(PHPINC) -c -o $(NAME).o libvirt.c $(XMLCFLAGS)
	$(CC) -shared $(LIBS) -rdynamic -o $(NAME).so $(NAME).o -ldl -lvirt $(XMLLIBS)
	$(ECHO) "Extension compiled as $(NAME).so"

install-exec-local:
	mkdir -p $(DESTDIR)$(PHPEDIR)
	$(CP) $(NAME).so $(DESTDIR)$(PHPEDIR)
	mkdir -p $(DESTDIR)$(sysconfdir)/php.d
	$(ECHO) "extension=$(NAME)" > $(DESTDIR)$(sysconfdir)/php.d/libvirt-php.ini

uninstall-local:
	$(RM) -f $(sysconfdir)/php.d/$(NAME).ini
	$(RM) -f $(DESTDIR)$(PHPEDIR)/$(NAME).so

clean-temp:
	$(RM) -f *.o
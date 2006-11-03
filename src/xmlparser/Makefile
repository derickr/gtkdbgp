DEV_DIR=.
INCLUDES=-I$(DEV_DIR) -I/usr/include/libxml2
#-I/dat/dev/php/php-5.0dev -I/dat/dev/php/php-5.0dev/main \	-I/dat/dev/php/php-5.0dev/Zend -I/dat/dev/php/php-5.0dev/TSRM

all: xdebug_xml.o xdebug_str.o
	gcc -o xml_reader $(INCLUDES) xdebug_xml_reader.c $? -lxml2 -ggdb3 -Wall

xdebug_xml.o: $(DEV_DIR)/xdebug_xml.c
	gcc $(INCLUDES) -c $@ $?

xdebug_str.o: $(DEV_DIR)/xdebug_str.c
	gcc $(INCLUDES) -c $@ $?

clean:
	rm *.o xml_reader

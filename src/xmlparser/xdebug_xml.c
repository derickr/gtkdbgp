/*
   +----------------------------------------------------------------------+
   | Xdebug                                                               |
   +----------------------------------------------------------------------+
   | Copyright (c) 2002, 2003, 2004, 2005, 2006 Derick Rethans            |
   +----------------------------------------------------------------------+
   | This source file is subject to version 1.0 of the Xdebug license,    |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://xdebug.derickrethans.nl/license.php                           |
   | If you did not receive a copy of the Xdebug license and are unable   |
   | to obtain it through the world-wide-web, please send a note to       |
   | xdebug@derickrethans.nl so we can mail you a copy immediately.       |
   +----------------------------------------------------------------------+
   | Authors:  Derick Rethans <derick@xdebug.org>                         |
   +----------------------------------------------------------------------+
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "xdebug_mm.h"
#include "xdebug_str.h"
#include "xdebug_xml.h"

static void xdebug_xml_return_attribute(xdebug_xml_attribute* attr, xdebug_str* output)
{
	char *tmp;
	int newlen;

	xdebug_str_addl(output, " ", 1, 0);
	xdebug_str_add(output, attr->name, 0);
	xdebug_str_addl(output, "=\"", 2, 0);
	if (attr->value) {
//		tmp = xmlize(attr->value, strlen(attr->value), &newlen);
		xdebug_str_add(output, attr->value, 0);
//		efree(tmp);
	}
	xdebug_str_addl(output, "\"", 1, 0);
	
	if (attr->next) {
		xdebug_xml_return_attribute(attr->next, output);
	}
}

static void xdebug_xml_return_text_node(xdebug_xml_text_node* node, xdebug_str* output)
{
	xdebug_str_addl(output, "<![CDATA[", 9, 0);
	xdebug_str_add(output, node->text, 0);
	xdebug_str_addl(output, "]]>", 3, 0);
}

void xdebug_xml_return_node(xdebug_xml_node* node, struct xdebug_str *output)
{
	xdebug_str_addl(output, "<", 1, 0);
	xdebug_str_add(output, node->tag, 0);

	if (node->text && node->text->encode) {
		xdebug_xml_add_attribute_ex(node, "encoding", "base64", 0, 0);
	}
	if (node->attribute) {
		xdebug_xml_return_attribute(node->attribute, output);
	}
	xdebug_str_addl(output, ">", 1, 0);

	if (node->child) {
		xdebug_xml_return_node(node->child, output);
	}

	if (node->text) {
		xdebug_xml_return_text_node(node->text, output);
	}

	xdebug_str_addl(output, "</", 2, 0);
	xdebug_str_add(output, node->tag, 0);
	xdebug_str_addl(output, ">", 1, 0);

	if (node->next) {
		xdebug_xml_return_node(node->next, output);
	}
}

xdebug_xml_node *xdebug_xml_node_init_ex(char *tag, int free_tag)
{
	xdebug_xml_node *xml = xdmalloc(sizeof (xdebug_xml_node));

	xml->tag = tag;
	xml->text = NULL;
	xml->child = NULL;
	xml->attribute = NULL;
	xml->next = NULL;
	xml->free_tag = free_tag;

	return xml;
}

void xdebug_xml_add_attribute_ex(xdebug_xml_node* xml, char *attribute, char *value, int free_name, int free_value)
{
	xdebug_xml_attribute *attr = xdmalloc(sizeof (xdebug_xml_attribute));
	xdebug_xml_attribute **ptr;

	/* Init structure */
	attr->name = attribute;
	attr->value = value;
	attr->next = NULL;
	attr->free_name = free_name;
	attr->free_value = free_value;

	/* Find last attribute in node */
	ptr = &xml->attribute;
	while (*ptr != NULL) {
		ptr = &(*ptr)->next;
	}
	*ptr = attr;
}

void xdebug_xml_add_child(xdebug_xml_node *xml, xdebug_xml_node *child)
{
	xdebug_xml_node **ptr;

	ptr = &xml->child;
	while (*ptr != NULL) {
		ptr = &((*ptr)->next);
	}
	*ptr = child;
}

static void xdebug_xml_text_node_dtor(xdebug_xml_text_node* node)
{
	if (node->free_value && node->text) {
		xdfree(node->text);
	}
	xdfree(node);
}

void xdebug_xml_add_text_ex(xdebug_xml_node *xml, char *text, int free_text, int encode)
{
	xdebug_xml_text_node *node = xdmalloc(sizeof (xdebug_xml_text_node));
	node->free_value = free_text;
	node->encode = encode;
	
	if (xml->text) {
		xdebug_xml_text_node_dtor(xml->text);
	}
	node->text = text;
	xml->text = node;
	if (!encode && strstr(node->text, "]]>")) {
		node->encode = 1;
	}
}

static void xdebug_xml_attribute_dtor(xdebug_xml_attribute *attr)
{
	if (attr->next) {
		xdebug_xml_attribute_dtor(attr->next);
	}
	if (attr->free_name) {
		xdfree(attr->name);
	}
	if (attr->free_value) {
		xdfree(attr->value);
	}
	xdfree(attr);
}

void xdebug_xml_node_dtor(xdebug_xml_node* xml)
{
	if (xml->next) {
		xdebug_xml_node_dtor(xml->next);
	}
	if (xml->child) {
		xdebug_xml_node_dtor(xml->child);
	}
	if (xml->attribute) {
		xdebug_xml_attribute_dtor(xml->attribute);
	}
	if (xml->free_tag) {
		xdfree(xml->tag);
	}
	if (xml->text) {
		xdebug_xml_text_node_dtor(xml->text);
	}
	xdfree(xml);
}

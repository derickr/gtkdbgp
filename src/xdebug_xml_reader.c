#define _GNU_SOURCE
#include <string.h>
#include <libxml/globals.h>
#include <libxml/parser.h>

#include "xdebug_xml.h"

typedef struct _xdebug_xml_reader_priv_struct xdebug_xml_reader_priv;

struct _xdebug_xml_reader_priv_struct {
	int level;
	xdebug_xml_node *xml;
	xdebug_xml_node *stack[32];
	xdebug_xml_node *current;
};

void xdebug_xml_start_element_handler(void *ctx, const xmlChar *name, const xmlChar **atts)
{
	xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr) ctx;
	xdebug_xml_reader_priv *data = (xdebug_xml_reader_priv *) ctxt->_private;
	xdebug_xml_node *child;

	data->stack[data->level] = data->current;
	data->level++;

	child = xdebug_xml_node_init_ex(xdstrdup(name), 1);
	if (data->current) {
		xdebug_xml_add_child(data->current, child);
	}
	data->current = child;
	if (!data->xml) {
		data->xml = child;
	}
	
	if (atts) {
		do {
			xdebug_xml_add_attribute_ex(child, xdstrdup(atts[0]), xdstrdup(atts[1]), 1, 1);
			atts = &atts[2];
		} while (atts[0]);
	}
}

void xdebug_xml_end_element_handler(void *ctx, const xmlChar *name)
{
	xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr) ctx;
	xdebug_xml_reader_priv *data = (xdebug_xml_reader_priv *) ctxt->_private;

	data->level--;
	data->current = data->stack[data->level];
}

void xdebug_xml_char_handler(void *ctx, const xmlChar *ch, int len)
{
	xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr) ctx;
	xdebug_xml_reader_priv *data = (xdebug_xml_reader_priv *) ctxt->_private;
	char *tmp;

	tmp = strndup(ch, len);
	xdebug_xml_add_text(data->current, xdstrdup(tmp));
	free(tmp);
}

xmlSAXHandler sax_handler = {
	/* internalSubsetDebug, */ NULL,
	/* isStandaloneDebug, */ NULL,
	/* hasInternalSubsetDebug, */ NULL,
	/* hasExternalSubsetDebug, */ NULL,
	/* resolveEntityDebug, */ NULL,
	/* getEntityDebug, */ NULL,
	/* entityDeclDebug, */ NULL,
	/* notationDeclDebug, */ NULL,
	/* attributeDeclDebug, */ NULL,
	/* elementDeclDebug, */ NULL,
	/* unparsedEntityDeclDebug, */ NULL,
	/* setDocumentLocatorDebug, */ NULL,
	/* startDocumentDebug, */ NULL,
	/* endDocumentDebug, */ NULL,
	xdebug_xml_start_element_handler,
	xdebug_xml_end_element_handler,
	/* referenceDebug, */ NULL,
	xdebug_xml_char_handler,
	/* ignorableWhitespaceDebug, */ NULL,
	/* processingInstructionDebug, */ NULL,
	/* commentDebug, */ NULL,
	/* warningDebug, */ NULL,
	/* errorDebug, */ NULL,
	/* fatalErrorDebug, */ NULL,
	/* getParameterEntityDebug, */ NULL,
	/* cdataBlockDebug, */ NULL,
	/* externalSubsetDebug, */ NULL,
	1
};

xmlSAXHandlerPtr sax_handler_ptr = &sax_handler;

xdebug_xml_node *parse_message(char *buffer, int bcount)
{
	int ret;
	char chars[10];
	xmlParserCtxtPtr ctxt;
	xdebug_str message = {0, 0, NULL};

	xdebug_xml_reader_priv data;
	data.level   = 0;
	data.xml     = NULL;
	data.current = NULL;

	ctxt = xmlCreatePushParserCtxt(sax_handler_ptr, NULL, NULL, 0, "test");
	ctxt->_private = &data;
	xmlParseChunk(ctxt, buffer, bcount, 1);

	if (data.xml) {
		xdebug_xml_return_node(data.xml, &message);
		printf("[%s]\n", message.d);
		xdebug_str_dtor(message);
	}
	
	xmlFreeParserCtxt(ctxt);
	xmlCleanupParser();
	xmlMemoryDump();

	return data.xml;
}

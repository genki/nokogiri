#include <xslt_stylesheet.h>

#include <libxslt/xsltInternals.h>
#include <libxslt/xsltutils.h>
#include <libxslt/transform.h>
#include <libexslt/exslt.h>

static void dealloc(xsltStylesheetPtr doc)
{
    NOKOGIRI_DEBUG_START(doc);
    xsltFreeStylesheet(doc); // commented out for now.
    NOKOGIRI_DEBUG_END(doc);
}

/*
 * call-seq:
 *   parse_stylesheet_doc(document)
 *
 * Parse a stylesheet from +document+.
 */
static VALUE parse_stylesheet_doc(VALUE klass, VALUE xmldocobj)
{
    xmlDocPtr xml ;
    xsltStylesheetPtr ss ;
    Data_Get_Struct(xmldocobj, xmlDoc, xml);
    exsltRegisterAll();
    ss = xsltParseStylesheetDoc(xmlCopyDoc(xml, 1)); /* 1 => recursive */
    return Data_Wrap_Struct(klass, NULL, dealloc, ss);
}


/*
 * call-seq:
 *   serialize(document)
 *
 * Serialize +document+ to an xml string.
 */
static VALUE serialize(VALUE self, VALUE xmlobj)
{
    xmlDocPtr xml ;
    xsltStylesheetPtr ss ;
    xmlChar* doc_ptr ;
    int doc_len ;
    VALUE rval ;

    Data_Get_Struct(xmlobj, xmlDoc, xml);
    Data_Get_Struct(self, xsltStylesheet, ss);
    xsltSaveResultToString(&doc_ptr, &doc_len, xml, ss);
    rval = NOKOGIRI_STR_NEW(doc_ptr, doc_len, xml->encoding);
    xmlFree(doc_ptr);
    return rval ;
}

/*
 *  call-seq:
 *    transform(document, params = [])
 *
 *  Apply an XSLT stylesheet to an XML::Document.
 *  +params+ is an array of strings used as XSLT parameters.
 *  returns Nokogiri::XML::Document
 *
 *  Example:
 * 
 *    doc   = Nokogiri::XML(File.read(ARGV[0]))
 *    xslt  = Nokogiri::XSLT(File.read(ARGV[1]))
 *    puts xslt.transform(doc, ['key', 'value'])
 *
 */
static VALUE transform(int argc, VALUE* argv, VALUE self)
{
    VALUE xmldoc, paramobj ;
    xmlDocPtr xml ;
    xmlDocPtr result ;
    xsltStylesheetPtr ss ;
    const char** params ;
    int param_len, j ;

    rb_scan_args(argc, argv, "11", &xmldoc, &paramobj);
    if (paramobj == Qnil) { paramobj = rb_ary_new2(0) ; }

    Data_Get_Struct(xmldoc, xmlDoc, xml);
    Data_Get_Struct(self, xsltStylesheet, ss);

    param_len = NUM2INT(rb_funcall(paramobj, rb_intern("length"), 0));
    params = calloc((size_t)param_len+1, sizeof(char*));
    for (j = 0 ; j < param_len ; j++) {
      VALUE entry = rb_ary_entry(paramobj, j);
      const char * ptr = StringValuePtr(entry);
      params[j] = ptr;
    }
    params[param_len] = 0 ;

    result = xsltApplyStylesheet(ss, xml, params);
    free(params);

    return Nokogiri_wrap_xml_document(0, result) ;
}

VALUE cNokogiriXsltStylesheet ;
void init_xslt_stylesheet()
{
  VALUE nokogiri = rb_define_module("Nokogiri");
  VALUE xslt = rb_define_module_under(nokogiri, "XSLT");
  VALUE klass = rb_define_class_under(xslt, "Stylesheet", rb_cObject);

  cNokogiriXsltStylesheet = klass;
    
  rb_define_singleton_method(klass, "parse_stylesheet_doc", parse_stylesheet_doc, 1);
  rb_define_method(klass, "serialize", serialize, 1);
  rb_define_method(klass, "transform", transform, -1);
}

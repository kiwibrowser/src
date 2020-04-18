// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include <memory>

#include "third_party/base/ptr_util.h"
#include "xfa/fxfa/parser/cxfa_accessiblecontent.h"
#include "xfa/fxfa/parser/cxfa_acrobat.h"
#include "xfa/fxfa/parser/cxfa_acrobat7.h"
#include "xfa/fxfa/parser/cxfa_adbe_jsconsole.h"
#include "xfa/fxfa/parser/cxfa_adbe_jsdebugger.h"
#include "xfa/fxfa/parser/cxfa_addsilentprint.h"
#include "xfa/fxfa/parser/cxfa_addviewerpreferences.h"
#include "xfa/fxfa/parser/cxfa_adjustdata.h"
#include "xfa/fxfa/parser/cxfa_adobeextensionlevel.h"
#include "xfa/fxfa/parser/cxfa_agent.h"
#include "xfa/fxfa/parser/cxfa_alwaysembed.h"
#include "xfa/fxfa/parser/cxfa_amd.h"
#include "xfa/fxfa/parser/cxfa_appearancefilter.h"
#include "xfa/fxfa/parser/cxfa_arc.h"
#include "xfa/fxfa/parser/cxfa_area.h"
#include "xfa/fxfa/parser/cxfa_assist.h"
#include "xfa/fxfa/parser/cxfa_attributes.h"
#include "xfa/fxfa/parser/cxfa_autosave.h"
#include "xfa/fxfa/parser/cxfa_barcode.h"
#include "xfa/fxfa/parser/cxfa_base.h"
#include "xfa/fxfa/parser/cxfa_batchoutput.h"
#include "xfa/fxfa/parser/cxfa_behavioroverride.h"
#include "xfa/fxfa/parser/cxfa_bind.h"
#include "xfa/fxfa/parser/cxfa_binditems.h"
#include "xfa/fxfa/parser/cxfa_bookend.h"
#include "xfa/fxfa/parser/cxfa_boolean.h"
#include "xfa/fxfa/parser/cxfa_border.h"
#include "xfa/fxfa/parser/cxfa_break.h"
#include "xfa/fxfa/parser/cxfa_breakafter.h"
#include "xfa/fxfa/parser/cxfa_breakbefore.h"
#include "xfa/fxfa/parser/cxfa_button.h"
#include "xfa/fxfa/parser/cxfa_cache.h"
#include "xfa/fxfa/parser/cxfa_calculate.h"
#include "xfa/fxfa/parser/cxfa_calendarsymbols.h"
#include "xfa/fxfa/parser/cxfa_caption.h"
#include "xfa/fxfa/parser/cxfa_certificate.h"
#include "xfa/fxfa/parser/cxfa_certificates.h"
#include "xfa/fxfa/parser/cxfa_change.h"
#include "xfa/fxfa/parser/cxfa_checkbutton.h"
#include "xfa/fxfa/parser/cxfa_choicelist.h"
#include "xfa/fxfa/parser/cxfa_color.h"
#include "xfa/fxfa/parser/cxfa_comb.h"
#include "xfa/fxfa/parser/cxfa_command.h"
#include "xfa/fxfa/parser/cxfa_common.h"
#include "xfa/fxfa/parser/cxfa_compress.h"
#include "xfa/fxfa/parser/cxfa_compression.h"
#include "xfa/fxfa/parser/cxfa_compresslogicalstructure.h"
#include "xfa/fxfa/parser/cxfa_compressobjectstream.h"
#include "xfa/fxfa/parser/cxfa_config.h"
#include "xfa/fxfa/parser/cxfa_conformance.h"
#include "xfa/fxfa/parser/cxfa_connect.h"
#include "xfa/fxfa/parser/cxfa_connectionset.h"
#include "xfa/fxfa/parser/cxfa_connectstring.h"
#include "xfa/fxfa/parser/cxfa_contentarea.h"
#include "xfa/fxfa/parser/cxfa_contentcopy.h"
#include "xfa/fxfa/parser/cxfa_copies.h"
#include "xfa/fxfa/parser/cxfa_corner.h"
#include "xfa/fxfa/parser/cxfa_creator.h"
#include "xfa/fxfa/parser/cxfa_currencysymbol.h"
#include "xfa/fxfa/parser/cxfa_currencysymbols.h"
#include "xfa/fxfa/parser/cxfa_currentpage.h"
#include "xfa/fxfa/parser/cxfa_data.h"
#include "xfa/fxfa/parser/cxfa_datagroup.h"
#include "xfa/fxfa/parser/cxfa_datamodel.h"
#include "xfa/fxfa/parser/cxfa_datavalue.h"
#include "xfa/fxfa/parser/cxfa_date.h"
#include "xfa/fxfa/parser/cxfa_datepattern.h"
#include "xfa/fxfa/parser/cxfa_datepatterns.h"
#include "xfa/fxfa/parser/cxfa_datetime.h"
#include "xfa/fxfa/parser/cxfa_datetimeedit.h"
#include "xfa/fxfa/parser/cxfa_datetimesymbols.h"
#include "xfa/fxfa/parser/cxfa_day.h"
#include "xfa/fxfa/parser/cxfa_daynames.h"
#include "xfa/fxfa/parser/cxfa_debug.h"
#include "xfa/fxfa/parser/cxfa_decimal.h"
#include "xfa/fxfa/parser/cxfa_defaulttypeface.h"
#include "xfa/fxfa/parser/cxfa_defaultui.h"
#include "xfa/fxfa/parser/cxfa_delete.h"
#include "xfa/fxfa/parser/cxfa_delta.h"
#include "xfa/fxfa/parser/cxfa_deltas.h"
#include "xfa/fxfa/parser/cxfa_desc.h"
#include "xfa/fxfa/parser/cxfa_destination.h"
#include "xfa/fxfa/parser/cxfa_digestmethod.h"
#include "xfa/fxfa/parser/cxfa_digestmethods.h"
#include "xfa/fxfa/parser/cxfa_documentassembly.h"
#include "xfa/fxfa/parser/cxfa_draw.h"
#include "xfa/fxfa/parser/cxfa_driver.h"
#include "xfa/fxfa/parser/cxfa_dsigdata.h"
#include "xfa/fxfa/parser/cxfa_duplexoption.h"
#include "xfa/fxfa/parser/cxfa_dynamicrender.h"
#include "xfa/fxfa/parser/cxfa_edge.h"
#include "xfa/fxfa/parser/cxfa_effectiveinputpolicy.h"
#include "xfa/fxfa/parser/cxfa_effectiveoutputpolicy.h"
#include "xfa/fxfa/parser/cxfa_embed.h"
#include "xfa/fxfa/parser/cxfa_encoding.h"
#include "xfa/fxfa/parser/cxfa_encodings.h"
#include "xfa/fxfa/parser/cxfa_encrypt.h"
#include "xfa/fxfa/parser/cxfa_encryption.h"
#include "xfa/fxfa/parser/cxfa_encryptionlevel.h"
#include "xfa/fxfa/parser/cxfa_encryptionmethod.h"
#include "xfa/fxfa/parser/cxfa_encryptionmethods.h"
#include "xfa/fxfa/parser/cxfa_enforce.h"
#include "xfa/fxfa/parser/cxfa_equate.h"
#include "xfa/fxfa/parser/cxfa_equaterange.h"
#include "xfa/fxfa/parser/cxfa_era.h"
#include "xfa/fxfa/parser/cxfa_eranames.h"
#include "xfa/fxfa/parser/cxfa_event.h"
#include "xfa/fxfa/parser/cxfa_exclgroup.h"
#include "xfa/fxfa/parser/cxfa_exclude.h"
#include "xfa/fxfa/parser/cxfa_excludens.h"
#include "xfa/fxfa/parser/cxfa_exdata.h"
#include "xfa/fxfa/parser/cxfa_execute.h"
#include "xfa/fxfa/parser/cxfa_exobject.h"
#include "xfa/fxfa/parser/cxfa_extras.h"
#include "xfa/fxfa/parser/cxfa_field.h"
#include "xfa/fxfa/parser/cxfa_fill.h"
#include "xfa/fxfa/parser/cxfa_filter.h"
#include "xfa/fxfa/parser/cxfa_fliplabel.h"
#include "xfa/fxfa/parser/cxfa_float.h"
#include "xfa/fxfa/parser/cxfa_font.h"
#include "xfa/fxfa/parser/cxfa_fontinfo.h"
#include "xfa/fxfa/parser/cxfa_form.h"
#include "xfa/fxfa/parser/cxfa_format.h"
#include "xfa/fxfa/parser/cxfa_formfieldfilling.h"
#include "xfa/fxfa/parser/cxfa_groupparent.h"
#include "xfa/fxfa/parser/cxfa_handler.h"
#include "xfa/fxfa/parser/cxfa_hyphenation.h"
#include "xfa/fxfa/parser/cxfa_ifempty.h"
#include "xfa/fxfa/parser/cxfa_image.h"
#include "xfa/fxfa/parser/cxfa_imageedit.h"
#include "xfa/fxfa/parser/cxfa_includexdpcontent.h"
#include "xfa/fxfa/parser/cxfa_incrementalload.h"
#include "xfa/fxfa/parser/cxfa_incrementalmerge.h"
#include "xfa/fxfa/parser/cxfa_insert.h"
#include "xfa/fxfa/parser/cxfa_instancemanager.h"
#include "xfa/fxfa/parser/cxfa_integer.h"
#include "xfa/fxfa/parser/cxfa_interactive.h"
#include "xfa/fxfa/parser/cxfa_issuers.h"
#include "xfa/fxfa/parser/cxfa_items.h"
#include "xfa/fxfa/parser/cxfa_jog.h"
#include "xfa/fxfa/parser/cxfa_keep.h"
#include "xfa/fxfa/parser/cxfa_keyusage.h"
#include "xfa/fxfa/parser/cxfa_labelprinter.h"
#include "xfa/fxfa/parser/cxfa_layout.h"
#include "xfa/fxfa/parser/cxfa_level.h"
#include "xfa/fxfa/parser/cxfa_line.h"
#include "xfa/fxfa/parser/cxfa_linear.h"
#include "xfa/fxfa/parser/cxfa_linearized.h"
#include "xfa/fxfa/parser/cxfa_locale.h"
#include "xfa/fxfa/parser/cxfa_localeset.h"
#include "xfa/fxfa/parser/cxfa_lockdocument.h"
#include "xfa/fxfa/parser/cxfa_log.h"
#include "xfa/fxfa/parser/cxfa_manifest.h"
#include "xfa/fxfa/parser/cxfa_map.h"
#include "xfa/fxfa/parser/cxfa_margin.h"
#include "xfa/fxfa/parser/cxfa_mdp.h"
#include "xfa/fxfa/parser/cxfa_medium.h"
#include "xfa/fxfa/parser/cxfa_mediuminfo.h"
#include "xfa/fxfa/parser/cxfa_meridiem.h"
#include "xfa/fxfa/parser/cxfa_meridiemnames.h"
#include "xfa/fxfa/parser/cxfa_message.h"
#include "xfa/fxfa/parser/cxfa_messaging.h"
#include "xfa/fxfa/parser/cxfa_mode.h"
#include "xfa/fxfa/parser/cxfa_modifyannots.h"
#include "xfa/fxfa/parser/cxfa_month.h"
#include "xfa/fxfa/parser/cxfa_monthnames.h"
#include "xfa/fxfa/parser/cxfa_msgid.h"
#include "xfa/fxfa/parser/cxfa_nameattr.h"
#include "xfa/fxfa/parser/cxfa_neverembed.h"
#include "xfa/fxfa/parser/cxfa_node.h"
#include "xfa/fxfa/parser/cxfa_numberofcopies.h"
#include "xfa/fxfa/parser/cxfa_numberpattern.h"
#include "xfa/fxfa/parser/cxfa_numberpatterns.h"
#include "xfa/fxfa/parser/cxfa_numbersymbol.h"
#include "xfa/fxfa/parser/cxfa_numbersymbols.h"
#include "xfa/fxfa/parser/cxfa_numericedit.h"
#include "xfa/fxfa/parser/cxfa_occur.h"
#include "xfa/fxfa/parser/cxfa_oid.h"
#include "xfa/fxfa/parser/cxfa_oids.h"
#include "xfa/fxfa/parser/cxfa_openaction.h"
#include "xfa/fxfa/parser/cxfa_operation.h"
#include "xfa/fxfa/parser/cxfa_output.h"
#include "xfa/fxfa/parser/cxfa_outputbin.h"
#include "xfa/fxfa/parser/cxfa_outputxsl.h"
#include "xfa/fxfa/parser/cxfa_overflow.h"
#include "xfa/fxfa/parser/cxfa_overprint.h"
#include "xfa/fxfa/parser/cxfa_packet.h"
#include "xfa/fxfa/parser/cxfa_packets.h"
#include "xfa/fxfa/parser/cxfa_pagearea.h"
#include "xfa/fxfa/parser/cxfa_pageoffset.h"
#include "xfa/fxfa/parser/cxfa_pagerange.h"
#include "xfa/fxfa/parser/cxfa_pageset.h"
#include "xfa/fxfa/parser/cxfa_pagination.h"
#include "xfa/fxfa/parser/cxfa_paginationoverride.h"
#include "xfa/fxfa/parser/cxfa_para.h"
#include "xfa/fxfa/parser/cxfa_part.h"
#include "xfa/fxfa/parser/cxfa_password.h"
#include "xfa/fxfa/parser/cxfa_passwordedit.h"
#include "xfa/fxfa/parser/cxfa_pattern.h"
#include "xfa/fxfa/parser/cxfa_pcl.h"
#include "xfa/fxfa/parser/cxfa_pdf.h"
#include "xfa/fxfa/parser/cxfa_pdfa.h"
#include "xfa/fxfa/parser/cxfa_permissions.h"
#include "xfa/fxfa/parser/cxfa_picktraybypdfsize.h"
#include "xfa/fxfa/parser/cxfa_picture.h"
#include "xfa/fxfa/parser/cxfa_plaintextmetadata.h"
#include "xfa/fxfa/parser/cxfa_presence.h"
#include "xfa/fxfa/parser/cxfa_present.h"
#include "xfa/fxfa/parser/cxfa_print.h"
#include "xfa/fxfa/parser/cxfa_printername.h"
#include "xfa/fxfa/parser/cxfa_printhighquality.h"
#include "xfa/fxfa/parser/cxfa_printscaling.h"
#include "xfa/fxfa/parser/cxfa_producer.h"
#include "xfa/fxfa/parser/cxfa_proto.h"
#include "xfa/fxfa/parser/cxfa_ps.h"
#include "xfa/fxfa/parser/cxfa_psmap.h"
#include "xfa/fxfa/parser/cxfa_query.h"
#include "xfa/fxfa/parser/cxfa_radial.h"
#include "xfa/fxfa/parser/cxfa_range.h"
#include "xfa/fxfa/parser/cxfa_reason.h"
#include "xfa/fxfa/parser/cxfa_reasons.h"
#include "xfa/fxfa/parser/cxfa_record.h"
#include "xfa/fxfa/parser/cxfa_recordset.h"
#include "xfa/fxfa/parser/cxfa_rectangle.h"
#include "xfa/fxfa/parser/cxfa_ref.h"
#include "xfa/fxfa/parser/cxfa_relevant.h"
#include "xfa/fxfa/parser/cxfa_rename.h"
#include "xfa/fxfa/parser/cxfa_renderpolicy.h"
#include "xfa/fxfa/parser/cxfa_rootelement.h"
#include "xfa/fxfa/parser/cxfa_runscripts.h"
#include "xfa/fxfa/parser/cxfa_script.h"
#include "xfa/fxfa/parser/cxfa_scriptmodel.h"
#include "xfa/fxfa/parser/cxfa_select.h"
#include "xfa/fxfa/parser/cxfa_setproperty.h"
#include "xfa/fxfa/parser/cxfa_severity.h"
#include "xfa/fxfa/parser/cxfa_sharptext.h"
#include "xfa/fxfa/parser/cxfa_sharpxhtml.h"
#include "xfa/fxfa/parser/cxfa_sharpxml.h"
#include "xfa/fxfa/parser/cxfa_signature.h"
#include "xfa/fxfa/parser/cxfa_signatureproperties.h"
#include "xfa/fxfa/parser/cxfa_signdata.h"
#include "xfa/fxfa/parser/cxfa_signing.h"
#include "xfa/fxfa/parser/cxfa_silentprint.h"
#include "xfa/fxfa/parser/cxfa_soapaction.h"
#include "xfa/fxfa/parser/cxfa_soapaddress.h"
#include "xfa/fxfa/parser/cxfa_solid.h"
#include "xfa/fxfa/parser/cxfa_source.h"
#include "xfa/fxfa/parser/cxfa_sourceset.h"
#include "xfa/fxfa/parser/cxfa_speak.h"
#include "xfa/fxfa/parser/cxfa_staple.h"
#include "xfa/fxfa/parser/cxfa_startnode.h"
#include "xfa/fxfa/parser/cxfa_startpage.h"
#include "xfa/fxfa/parser/cxfa_stipple.h"
#include "xfa/fxfa/parser/cxfa_subform.h"
#include "xfa/fxfa/parser/cxfa_subformset.h"
#include "xfa/fxfa/parser/cxfa_subjectdn.h"
#include "xfa/fxfa/parser/cxfa_subjectdns.h"
#include "xfa/fxfa/parser/cxfa_submit.h"
#include "xfa/fxfa/parser/cxfa_submitformat.h"
#include "xfa/fxfa/parser/cxfa_submiturl.h"
#include "xfa/fxfa/parser/cxfa_subsetbelow.h"
#include "xfa/fxfa/parser/cxfa_suppressbanner.h"
#include "xfa/fxfa/parser/cxfa_tagged.h"
#include "xfa/fxfa/parser/cxfa_template.h"
#include "xfa/fxfa/parser/cxfa_templatecache.h"
#include "xfa/fxfa/parser/cxfa_text.h"
#include "xfa/fxfa/parser/cxfa_textedit.h"
#include "xfa/fxfa/parser/cxfa_threshold.h"
#include "xfa/fxfa/parser/cxfa_time.h"
#include "xfa/fxfa/parser/cxfa_timepattern.h"
#include "xfa/fxfa/parser/cxfa_timepatterns.h"
#include "xfa/fxfa/parser/cxfa_timestamp.h"
#include "xfa/fxfa/parser/cxfa_to.h"
#include "xfa/fxfa/parser/cxfa_tooltip.h"
#include "xfa/fxfa/parser/cxfa_trace.h"
#include "xfa/fxfa/parser/cxfa_transform.h"
#include "xfa/fxfa/parser/cxfa_traversal.h"
#include "xfa/fxfa/parser/cxfa_traverse.h"
#include "xfa/fxfa/parser/cxfa_type.h"
#include "xfa/fxfa/parser/cxfa_typeface.h"
#include "xfa/fxfa/parser/cxfa_typefaces.h"
#include "xfa/fxfa/parser/cxfa_ui.h"
#include "xfa/fxfa/parser/cxfa_update.h"
#include "xfa/fxfa/parser/cxfa_uri.h"
#include "xfa/fxfa/parser/cxfa_user.h"
#include "xfa/fxfa/parser/cxfa_validate.h"
#include "xfa/fxfa/parser/cxfa_validateapprovalsignatures.h"
#include "xfa/fxfa/parser/cxfa_validationmessaging.h"
#include "xfa/fxfa/parser/cxfa_value.h"
#include "xfa/fxfa/parser/cxfa_variables.h"
#include "xfa/fxfa/parser/cxfa_version.h"
#include "xfa/fxfa/parser/cxfa_versioncontrol.h"
#include "xfa/fxfa/parser/cxfa_viewerpreferences.h"
#include "xfa/fxfa/parser/cxfa_webclient.h"
#include "xfa/fxfa/parser/cxfa_whitespace.h"
#include "xfa/fxfa/parser/cxfa_window.h"
#include "xfa/fxfa/parser/cxfa_wsdladdress.h"
#include "xfa/fxfa/parser/cxfa_wsdlconnection.h"
#include "xfa/fxfa/parser/cxfa_xdc.h"
#include "xfa/fxfa/parser/cxfa_xdp.h"
#include "xfa/fxfa/parser/cxfa_xfa.h"
#include "xfa/fxfa/parser/cxfa_xmlconnection.h"
#include "xfa/fxfa/parser/cxfa_xsdconnection.h"
#include "xfa/fxfa/parser/cxfa_xsl.h"
#include "xfa/fxfa/parser/cxfa_zpl.h"

namespace {

struct ElementNameInfo {
  uint32_t hash;
  XFA_Element element;
};

const ElementNameInfo ElementNameToEnum[] = {
    {0x23ee3 /* ps */, XFA_Element::Ps},
    {0x25363 /* to */, XFA_Element::To},
    {0x2587e /* ui */, XFA_Element::Ui},
    {0x1c648b /* recordSet */, XFA_Element::RecordSet},
    {0x171428f /* subsetBelow */, XFA_Element::SubsetBelow},
    {0x1a0776a /* subformSet */, XFA_Element::SubformSet},
    {0x2340d70 /* adobeExtensionLevel */, XFA_Element::AdobeExtensionLevel},
    {0x2c1c7f1 /* typeface */, XFA_Element::Typeface},
    {0x5518c25 /* break */, XFA_Element::Break},
    {0x5fff523 /* fontInfo */, XFA_Element::FontInfo},
    {0x653a227 /* numberPattern */, XFA_Element::NumberPattern},
    {0x65b4a05 /* dynamicRender */, XFA_Element::DynamicRender},
    {0x7e4362e /* printScaling */, XFA_Element::PrintScaling},
    {0x7fe6d3a /* checkButton */, XFA_Element::CheckButton},
    {0x80cf58f /* datePatterns */, XFA_Element::DatePatterns},
    {0x811929d /* sourceSet */, XFA_Element::SourceSet},
    {0x9f9d612 /* amd */, XFA_Element::Amd},
    {0x9f9efb6 /* arc */, XFA_Element::Arc},
    {0xa48835e /* day */, XFA_Element::Day},
    {0xa6328b8 /* era */, XFA_Element::Era},
    {0xae6a0a0 /* jog */, XFA_Element::Jog},
    {0xb1b3d22 /* log */, XFA_Element::Log},
    {0xb35439e /* map */, XFA_Element::Map},
    {0xb355301 /* mdp */, XFA_Element::Mdp},
    {0xb420438 /* breakBefore */, XFA_Element::BreakBefore},
    {0xb6a091c /* oid */, XFA_Element::Oid},
    {0xb84389f /* pcl */, XFA_Element::Pcl},
    {0xb843dba /* pdf */, XFA_Element::Pdf},
    {0xbb8df5d /* ref */, XFA_Element::Ref},
    {0xc080cd0 /* uri */, XFA_Element::Uri},
    {0xc56afbf /* xdc */, XFA_Element::Xdc},
    {0xc56afcc /* xdp */, XFA_Element::Xdp},
    {0xc56b9ff /* xfa */, XFA_Element::Xfa},
    {0xc56fcb7 /* xsl */, XFA_Element::Xsl},
    {0xc8b89d6 /* zpl */, XFA_Element::Zpl},
    {0xc9bae94 /* cache */, XFA_Element::Cache},
    {0xcb016be /* margin */, XFA_Element::Margin},
    {0xe1378fe /* keyUsage */, XFA_Element::KeyUsage},
    {0xfe3596a /* exclude */, XFA_Element::Exclude},
    {0x10395ac7 /* choiceList */, XFA_Element::ChoiceList},
    {0x1059ec18 /* level */, XFA_Element::Level},
    {0x10874804 /* labelPrinter */, XFA_Element::LabelPrinter},
    {0x10c40e03 /* calendarSymbols */, XFA_Element::CalendarSymbols},
    {0x10f1ea24 /* para */, XFA_Element::Para},
    {0x10f1ea37 /* part */, XFA_Element::Part},
    {0x1140975b /* pdfa */, XFA_Element::Pdfa},
    {0x1154efe6 /* filter */, XFA_Element::Filter},
    {0x13f41de1 /* present */, XFA_Element::Present},
    {0x1827e6ea /* pagination */, XFA_Element::Pagination},
    {0x18463707 /* encoding */, XFA_Element::Encoding},
    {0x185e41e2 /* event */, XFA_Element::Event},
    {0x1adb142d /* whitespace */, XFA_Element::Whitespace},
    {0x1f3f64c3 /* defaultUi */, XFA_Element::DefaultUi},
    {0x204e87cb /* dataModel */, XFA_Element::DataModel},
    {0x2057b350 /* barcode */, XFA_Element::Barcode},
    {0x20596bad /* timePattern */, XFA_Element::TimePattern},
    {0x210b74d3 /* batchOutput */, XFA_Element::BatchOutput},
    {0x212ff0e2 /* enforce */, XFA_Element::Enforce},
    {0x21d351b4 /* currencySymbols */, XFA_Element::CurrencySymbols},
    {0x21db83c5 /* addSilentPrint */, XFA_Element::AddSilentPrint},
    {0x22266258 /* rename */, XFA_Element::Rename},
    {0x226ca8f1 /* operation */, XFA_Element::Operation},
    {0x23e27b84 /* typefaces */, XFA_Element::Typefaces},
    {0x23f4aa75 /* subjectDNs */, XFA_Element::SubjectDNs},
    {0x240d5e8e /* issuers */, XFA_Element::Issuers},
    {0x24a52f8a /* wsdlConnection */, XFA_Element::WsdlConnection},
    {0x254ebd07 /* debug */, XFA_Element::Debug},
    {0x2655c66a /* delta */, XFA_Element::Delta},
    {0x26c0daec /* eraNames */, XFA_Element::EraNames},
    {0x273ab03b /* modifyAnnots */, XFA_Element::ModifyAnnots},
    {0x27875bb4 /* startNode */, XFA_Element::StartNode},
    {0x285d0dbc /* button */, XFA_Element::Button},
    {0x28dee6e9 /* format */, XFA_Element::Format},
    {0x2a23349e /* border */, XFA_Element::Border},
    {0x2ae67f19 /* area */, XFA_Element::Area},
    {0x2c3c4c67 /* hyphenation */, XFA_Element::Hyphenation},
    {0x2d08af85 /* text */, XFA_Element::Text},
    {0x2d71b00f /* time */, XFA_Element::Time},
    {0x2f16a382 /* type */, XFA_Element::Type},
    {0x2fe057e9 /* overprint */, XFA_Element::Overprint},
    {0x302aee16 /* certificates */, XFA_Element::Certificates},
    {0x30b227df /* encryptionMethods */, XFA_Element::EncryptionMethods},
    {0x32b900d1 /* setProperty */, XFA_Element::SetProperty},
    {0x337d9e45 /* printerName */, XFA_Element::PrinterName},
    {0x33edda4b /* startPage */, XFA_Element::StartPage},
    {0x381943e4 /* pageOffset */, XFA_Element::PageOffset},
    {0x382106cd /* dateTime */, XFA_Element::DateTime},
    {0x386e7421 /* comb */, XFA_Element::Comb},
    {0x390acd9e /* pattern */, XFA_Element::Pattern},
    {0x3942163e /* ifEmpty */, XFA_Element::IfEmpty},
    {0x39944a7b /* suppressBanner */, XFA_Element::SuppressBanner},
    {0x3b3c3dca /* outputBin */, XFA_Element::OutputBin},
    {0x3b8a4024 /* field */, XFA_Element::Field},
    {0x3c15352f /* agent */, XFA_Element::Agent},
    {0x3d7e8668 /* outputXSL */, XFA_Element::OutputXSL},
    {0x3e1c91c5 /* adjustData */, XFA_Element::AdjustData},
    {0x3e7a9408 /* autoSave */, XFA_Element::AutoSave},
    {0x3ecead94 /* contentArea */, XFA_Element::ContentArea},
    {0x3fadaec0 /* wsdlAddress */, XFA_Element::WsdlAddress},
    {0x40623b5b /* solid */, XFA_Element::Solid},
    {0x41f0bd76 /* dateTimeSymbols */, XFA_Element::DateTimeSymbols},
    {0x444e7523 /* encryptionLevel */, XFA_Element::EncryptionLevel},
    {0x4523af55 /* edge */, XFA_Element::Edge},
    {0x45d5e3c1 /* stipple */, XFA_Element::Stipple},
    {0x475e4e87 /* attributes */, XFA_Element::Attributes},
    {0x487a8c87 /* versionControl */, XFA_Element::VersionControl},
    {0x48e5248c /* meridiem */, XFA_Element::Meridiem},
    {0x48f36719 /* exclGroup */, XFA_Element::ExclGroup},
    {0x4977356b /* toolTip */, XFA_Element::ToolTip},
    {0x499afecc /* compress */, XFA_Element::Compress},
    {0x4a0c4948 /* reason */, XFA_Element::Reason},
    {0x4bdcce13 /* execute */, XFA_Element::Execute},
    {0x4c56b216 /* contentCopy */, XFA_Element::ContentCopy},
    {0x4cc176d3 /* dateTimeEdit */, XFA_Element::DateTimeEdit},
    {0x4e1e39b6 /* config */, XFA_Element::Config},
    {0x4e2d6083 /* image */, XFA_Element::Image},
    {0x4e814150 /* #xHTML */, XFA_Element::SharpxHTML},
    {0x4f2388c1 /* numberOfCopies */, XFA_Element::NumberOfCopies},
    {0x4f512e30 /* behaviorOverride */, XFA_Element::BehaviorOverride},
    {0x4fdc3454 /* timeStamp */, XFA_Element::TimeStamp},
    {0x51d90546 /* month */, XFA_Element::Month},
    {0x523437e4 /* viewerPreferences */, XFA_Element::ViewerPreferences},
    {0x53abc1c6 /* scriptModel */, XFA_Element::ScriptModel},
    {0x54034c2f /* decimal */, XFA_Element::Decimal},
    {0x54202c9e /* subform */, XFA_Element::Subform},
    {0x542c7300 /* select */, XFA_Element::Select},
    {0x5436d198 /* window */, XFA_Element::Window},
    {0x5473b6dc /* localeSet */, XFA_Element::LocaleSet},
    {0x56ae179e /* handler */, XFA_Element::Handler},
    {0x570ce835 /* presence */, XFA_Element::Presence},
    {0x5779d65f /* record */, XFA_Element::Record},
    {0x59c8f27d /* embed */, XFA_Element::Embed},
    {0x5a50e9e6 /* version */, XFA_Element::Version},
    {0x5b8383df /* command */, XFA_Element::Command},
    {0x5c43c6c3 /* copies */, XFA_Element::Copies},
    {0x5e0c2c49 /* staple */, XFA_Element::Staple},
    {0x5e5083dd /* submitFormat */, XFA_Element::SubmitFormat},
    {0x5e8c5d20 /* boolean */, XFA_Element::Boolean},
    {0x60490a85 /* message */, XFA_Element::Message},
    {0x60d4c8b1 /* output */, XFA_Element::Output},
    {0x61810081 /* psMap */, XFA_Element::PsMap},
    {0x62bd904b /* excludeNS */, XFA_Element::ExcludeNS},
    {0x669d4f77 /* assist */, XFA_Element::Assist},
    {0x67334a1c /* picture */, XFA_Element::Picture},
    {0x67fe7334 /* traversal */, XFA_Element::Traversal},
    {0x6894589c /* silentPrint */, XFA_Element::SilentPrint},
    {0x68a16bbd /* webClient */, XFA_Element::WebClient},
    {0x6a4bc084 /* producer */, XFA_Element::Producer},
    {0x6a9e04c9 /* corner */, XFA_Element::Corner},
    {0x6ccd7274 /* msgId */, XFA_Element::MsgId},
    {0x6e67921f /* color */, XFA_Element::Color},
    {0x6ec217a5 /* keep */, XFA_Element::Keep},
    {0x6eef1116 /* query */, XFA_Element::Query},
    {0x7033bfd5 /* insert */, XFA_Element::Insert},
    {0x704af389 /* imageEdit */, XFA_Element::ImageEdit},
    {0x7233018a /* validate */, XFA_Element::Validate},
    {0x72ba47b4 /* digestMethods */, XFA_Element::DigestMethods},
    {0x72f2aa7a /* numberPatterns */, XFA_Element::NumberPatterns},
    {0x74caed29 /* pageSet */, XFA_Element::PageSet},
    {0x7568e6ae /* integer */, XFA_Element::Integer},
    {0x76182db9 /* soapAddress */, XFA_Element::SoapAddress},
    {0x773146c5 /* equate */, XFA_Element::Equate},
    {0x77d449dd /* formFieldFilling */, XFA_Element::FormFieldFilling},
    {0x7889d68a /* pageRange */, XFA_Element::PageRange},
    {0x7baca2e3 /* update */, XFA_Element::Update},
    {0x7ce89001 /* connectString */, XFA_Element::ConnectString},
    {0x7d9fd7c5 /* mode */, XFA_Element::Mode},
    {0x7e7e845e /* layout */, XFA_Element::Layout},
    {0x7e845c34 /* #xml */, XFA_Element::Sharpxml},
    {0x7fb341df /* xsdConnection */, XFA_Element::XsdConnection},
    {0x7ffb51cc /* traverse */, XFA_Element::Traverse},
    {0x80203b5a /* encodings */, XFA_Element::Encodings},
    {0x803550fc /* template */, XFA_Element::Template},
    {0x803d5bbc /* acrobat */, XFA_Element::Acrobat},
    {0x821d6569 /* validationMessaging */, XFA_Element::ValidationMessaging},
    {0x830e688f /* signing */, XFA_Element::Signing},
    {0x83dab9f5 /* script */, XFA_Element::Script},
    {0x8411ebcd /* addViewerPreferences */, XFA_Element::AddViewerPreferences},
    {0x8777642e /* alwaysEmbed */, XFA_Element::AlwaysEmbed},
    {0x877a6b39 /* passwordEdit */, XFA_Element::PasswordEdit},
    {0x87e84c99 /* numericEdit */, XFA_Element::NumericEdit},
    {0x8852cdec /* encryptionMethod */, XFA_Element::EncryptionMethod},
    {0x891f4606 /* change */, XFA_Element::Change},
    {0x89939f36 /* pageArea */, XFA_Element::PageArea},
    {0x8a9d6247 /* submitUrl */, XFA_Element::SubmitUrl},
    {0x8ad8b90f /* oids */, XFA_Element::Oids},
    {0x8b036f32 /* signature */, XFA_Element::Signature},
    {0x8b128efb /* ADBE_JSConsole */, XFA_Element::ADBE_JSConsole},
    {0x8bcfe96e /* caption */, XFA_Element::Caption},
    {0x8e1c2921 /* relevant */, XFA_Element::Relevant},
    {0x8e3f0a4b /* flipLabel */, XFA_Element::FlipLabel},
    {0x900280b7 /* exData */, XFA_Element::ExData},
    {0x91e80352 /* dayNames */, XFA_Element::DayNames},
    {0x93113b11 /* soapAction */, XFA_Element::SoapAction},
    {0x938b09f6 /* defaultTypeface */, XFA_Element::DefaultTypeface},
    {0x95b37897 /* manifest */, XFA_Element::Manifest},
    {0x97b76b54 /* overflow */, XFA_Element::Overflow},
    {0x9a57861b /* linear */, XFA_Element::Linear},
    {0x9ad5a821 /* currencySymbol */, XFA_Element::CurrencySymbol},
    {0x9c6471b3 /* delete */, XFA_Element::Delete},
    {0x9deea61d /* deltas */, XFA_Element::Deltas},
    {0x9e67de21 /* digestMethod */, XFA_Element::DigestMethod},
    {0x9f3e9510 /* instanceManager */, XFA_Element::InstanceManager},
    {0xa0799892 /* equateRange */, XFA_Element::EquateRange},
    {0xa084a381 /* medium */, XFA_Element::Medium},
    {0xa1211b8b /* textEdit */, XFA_Element::TextEdit},
    {0xa17008f0 /* templateCache */, XFA_Element::TemplateCache},
    {0xa4f7b88f /* compressObjectStream */, XFA_Element::CompressObjectStream},
    {0xa65f5d17 /* dataValue */, XFA_Element::DataValue},
    {0xa6caaa89 /* accessibleContent */, XFA_Element::AccessibleContent},
    {0xa94cc00b /* includeXDPContent */, XFA_Element::IncludeXDPContent},
    {0xa9b081a1 /* xmlConnection */, XFA_Element::XmlConnection},
    {0xab2a3b74 /* validateApprovalSignatures */,
     XFA_Element::ValidateApprovalSignatures},
    {0xab8c5a2b /* signData */, XFA_Element::SignData},
    {0xabaa2ceb /* packets */, XFA_Element::Packets},
    {0xadba359c /* datePattern */, XFA_Element::DatePattern},
    {0xae222b2b /* duplexOption */, XFA_Element::DuplexOption},
    {0xb012effb /* base */, XFA_Element::Base},
    {0xb0e5485d /* bind */, XFA_Element::Bind},
    {0xb45d61b2 /* compression */, XFA_Element::Compression},
    {0xb563f0ff /* user */, XFA_Element::User},
    {0xb5848ad5 /* rectangle */, XFA_Element::Rectangle},
    {0xb6dacb72 /* effectiveOutputPolicy */,
     XFA_Element::EffectiveOutputPolicy},
    {0xb7d7654d /* ADBE_JSDebugger */, XFA_Element::ADBE_JSDebugger},
    {0xbab37f73 /* acrobat7 */, XFA_Element::Acrobat7},
    {0xbc70081e /* interactive */, XFA_Element::Interactive},
    {0xbc8fa350 /* locale */, XFA_Element::Locale},
    {0xbcd44940 /* currentPage */, XFA_Element::CurrentPage},
    {0xbde9abda /* data */, XFA_Element::Data},
    {0xbde9abde /* date */, XFA_Element::Date},
    {0xbe52dfbf /* desc */, XFA_Element::Desc},
    {0xbf4b6405 /* encrypt */, XFA_Element::Encrypt},
    {0xbfa87cce /* draw */, XFA_Element::Draw},
    {0xc181ff4b /* encryption */, XFA_Element::Encryption},
    {0xc1970f40 /* meridiemNames */, XFA_Element::MeridiemNames},
    {0xc5ad9f5e /* messaging */, XFA_Element::Messaging},
    {0xc69549f4 /* speak */, XFA_Element::Speak},
    {0xc7743dc7 /* dataGroup */, XFA_Element::DataGroup},
    {0xc7eb20e9 /* common */, XFA_Element::Common},
    {0xc85d4528 /* #text */, XFA_Element::Sharptext},
    {0xc861556a /* paginationOverride */, XFA_Element::PaginationOverride},
    {0xc903dabb /* reasons */, XFA_Element::Reasons},
    {0xc9a8127f /* signatureProperties */, XFA_Element::SignatureProperties},
    {0xca010c2d /* threshold */, XFA_Element::Threshold},
    {0xcb4c5e96 /* appearanceFilter */, XFA_Element::AppearanceFilter},
    {0xcc92aba7 /* fill */, XFA_Element::Fill},
    {0xcd308b77 /* font */, XFA_Element::Font},
    {0xcd309ff4 /* form */, XFA_Element::Form},
    {0xcebcca2d /* mediumInfo */, XFA_Element::MediumInfo},
    {0xcfe0d643 /* certificate */, XFA_Element::Certificate},
    {0xd012c033 /* password */, XFA_Element::Password},
    {0xd01604bd /* runScripts */, XFA_Element::RunScripts},
    {0xd1227e6f /* trace */, XFA_Element::Trace},
    {0xd1532876 /* float */, XFA_Element::Float},
    {0xd17a6c30 /* renderPolicy */, XFA_Element::RenderPolicy},
    {0xd58aa962 /* destination */, XFA_Element::Destination},
    {0xd6e27f1d /* value */, XFA_Element::Value},
    {0xd7a14462 /* bookend */, XFA_Element::Bookend},
    {0xd8c31254 /* exObject */, XFA_Element::ExObject},
    {0xda6a8590 /* openAction */, XFA_Element::OpenAction},
    {0xdab4fb7d /* neverEmbed */, XFA_Element::NeverEmbed},
    {0xdb98475f /* bindItems */, XFA_Element::BindItems},
    {0xdbfbe02e /* calculate */, XFA_Element::Calculate},
    {0xdd7676ed /* print */, XFA_Element::Print},
    {0xdde273d7 /* extras */, XFA_Element::Extras},
    {0xde146b34 /* proto */, XFA_Element::Proto},
    {0xdf059321 /* dSigData */, XFA_Element::DSigData},
    {0xdfccf030 /* creator */, XFA_Element::Creator},
    {0xdff78c6a /* connect */, XFA_Element::Connect},
    {0xe11a2cbc /* permissions */, XFA_Element::Permissions},
    {0xe14c801c /* connectionSet */, XFA_Element::ConnectionSet},
    {0xe1c83a14 /* submit */, XFA_Element::Submit},
    {0xe29821cd /* range */, XFA_Element::Range},
    {0xe38d83c7 /* linearized */, XFA_Element::Linearized},
    {0xe3aa2578 /* packet */, XFA_Element::Packet},
    {0xe3aa860e /* rootElement */, XFA_Element::RootElement},
    {0xe3e553fa /* plaintextMetadata */, XFA_Element::PlaintextMetadata},
    {0xe3e6e4f2 /* numberSymbols */, XFA_Element::NumberSymbols},
    {0xe3f067f6 /* printHighQuality */, XFA_Element::PrintHighQuality},
    {0xe3fd078c /* driver */, XFA_Element::Driver},
    {0xe48b34f2 /* incrementalLoad */, XFA_Element::IncrementalLoad},
    {0xe550e7c2 /* subjectDN */, XFA_Element::SubjectDN},
    {0xe6669a78 /* compressLogicalStructure */,
     XFA_Element::CompressLogicalStructure},
    {0xe7a7ea02 /* incrementalMerge */, XFA_Element::IncrementalMerge},
    {0xe948530d /* radial */, XFA_Element::Radial},
    {0xea8d6999 /* variables */, XFA_Element::Variables},
    {0xeaa142c0 /* timePatterns */, XFA_Element::TimePatterns},
    {0xeb943a71 /* effectiveInputPolicy */, XFA_Element::EffectiveInputPolicy},
    {0xef04a2bc /* nameAttr */, XFA_Element::NameAttr},
    {0xf07222ab /* conformance */, XFA_Element::Conformance},
    {0xf0aaaadc /* transform */, XFA_Element::Transform},
    {0xf1433e88 /* lockDocument */, XFA_Element::LockDocument},
    {0xf54eb997 /* breakAfter */, XFA_Element::BreakAfter},
    {0xf616da28 /* line */, XFA_Element::Line},
    {0xf7055fb1 /* source */, XFA_Element::Source},
    {0xf7eebe1c /* occur */, XFA_Element::Occur},
    {0xf8d10d97 /* pickTrayByPDFSize */, XFA_Element::PickTrayByPDFSize},
    {0xf8f19e3a /* monthNames */, XFA_Element::MonthNames},
    {0xf984149b /* severity */, XFA_Element::Severity},
    {0xf9bcb037 /* groupParent */, XFA_Element::GroupParent},
    {0xfbc42fff /* documentAssembly */, XFA_Element::DocumentAssembly},
    {0xfc78159f /* numberSymbol */, XFA_Element::NumberSymbol},
    {0xfcbd606c /* tagged */, XFA_Element::Tagged},
    {0xff063802 /* items */, XFA_Element::Items},
};

struct AttributeNameInfo {
  uint32_t hash;
  XFA_Attribute attribute;
};

const AttributeNameInfo AttributeNameInfoToEnum[] = {
    {0x68 /* h */, XFA_Attribute::H},
    {0x77 /* w */, XFA_Attribute::W},
    {0x78 /* x */, XFA_Attribute::X},
    {0x79 /* y */, XFA_Attribute::Y},
    {0x21aed /* id */, XFA_Attribute::Id},
    {0x25363 /* to */, XFA_Attribute::To},
    {0xcb0ac9 /* lineThrough */, XFA_Attribute::LineThrough},
    {0x2282c73 /* hAlign */, XFA_Attribute::HAlign},
    {0x2c1c7f1 /* typeface */, XFA_Attribute::Typeface},
    {0x3106c3a /* beforeTarget */, XFA_Attribute::BeforeTarget},
    {0x31b19c1 /* name */, XFA_Attribute::Name},
    {0x3848b3f /* next */, XFA_Attribute::Next},
    {0x43e349b /* dataRowCount */, XFA_Attribute::DataRowCount},
    {0x5518c25 /* break */, XFA_Attribute::Break},
    {0x5ce6195 /* vScrollPolicy */, XFA_Attribute::VScrollPolicy},
    {0x8c74ae9 /* fontHorizontalScale */, XFA_Attribute::FontHorizontalScale},
    {0x8d4f1c7 /* textIndent */, XFA_Attribute::TextIndent},
    {0x97c1c65 /* context */, XFA_Attribute::Context},
    {0x9876578 /* trayOut */, XFA_Attribute::TrayOut},
    {0xa2e3514 /* cap */, XFA_Attribute::Cap},
    {0xb3543a6 /* max */, XFA_Attribute::Max},
    {0xb356ca4 /* min */, XFA_Attribute::Min},
    {0xbb8df5d /* ref */, XFA_Attribute::Ref},
    {0xbb8f3df /* rid */, XFA_Attribute::Rid},
    {0xc080cd3 /* url */, XFA_Attribute::Url},
    {0xc0811ed /* use */, XFA_Attribute::Use},
    {0xcfea02e /* leftInset */, XFA_Attribute::LeftInset},
    {0x1026c59d /* widows */, XFA_Attribute::Widows},
    {0x1059ec18 /* level */, XFA_Attribute::Level},
    {0x1356caf8 /* bottomInset */, XFA_Attribute::BottomInset},
    {0x13a08bdb /* overflowTarget */, XFA_Attribute::OverflowTarget},
    {0x1414d431 /* allowMacro */, XFA_Attribute::AllowMacro},
    {0x14a32d52 /* pagePosition */, XFA_Attribute::PagePosition},
    {0x1517dfa1 /* columnWidths */, XFA_Attribute::ColumnWidths},
    {0x169134a1 /* overflowLeader */, XFA_Attribute::OverflowLeader},
    {0x1b8dce3e /* action */, XFA_Attribute::Action},
    {0x1e459b8f /* nonRepudiation */, XFA_Attribute::NonRepudiation},
    {0x1ec8ab2c /* rate */, XFA_Attribute::Rate},
    {0x1ef3a64a /* allowRichText */, XFA_Attribute::AllowRichText},
    {0x2038c9b2 /* role */, XFA_Attribute::Role},
    {0x20914367 /* overflowTrailer */, XFA_Attribute::OverflowTrailer},
    {0x226ca8f1 /* operation */, XFA_Attribute::Operation},
    {0x24d85167 /* timeout */, XFA_Attribute::Timeout},
    {0x25764436 /* topInset */, XFA_Attribute::TopInset},
    {0x25839852 /* access */, XFA_Attribute::Access},
    {0x268b7ec1 /* commandType */, XFA_Attribute::CommandType},
    {0x28dee6e9 /* format */, XFA_Attribute::Format},
    {0x28e17e91 /* dataPrep */, XFA_Attribute::DataPrep},
    {0x292b88fe /* widgetData */, XFA_Attribute::WidgetData},
    {0x29418bb7 /* abbr */, XFA_Attribute::Abbr},
    {0x2a82d99c /* marginRight */, XFA_Attribute::MarginRight},
    {0x2b5df51e /* dataDescription */, XFA_Attribute::DataDescription},
    {0x2bb3f470 /* encipherOnly */, XFA_Attribute::EncipherOnly},
    {0x2cd79033 /* kerningMode */, XFA_Attribute::KerningMode},
    {0x2ee7678f /* rotate */, XFA_Attribute::Rotate},
    {0x2f105f72 /* wordCharacterCount */, XFA_Attribute::WordCharacterCount},
    {0x2f16a382 /* type */, XFA_Attribute::Type},
    {0x34ae103c /* reserve */, XFA_Attribute::Reserve},
    {0x3650557e /* textLocation */, XFA_Attribute::TextLocation},
    {0x39cdb0a2 /* priority */, XFA_Attribute::Priority},
    {0x3a0273a6 /* underline */, XFA_Attribute::Underline},
    {0x3b582286 /* moduleWidth */, XFA_Attribute::ModuleWidth},
    {0x3d123c26 /* hyphenate */, XFA_Attribute::Hyphenate},
    {0x3e7af94f /* listen */, XFA_Attribute::Listen},
    {0x4156ee3f /* delimiter */, XFA_Attribute::Delimiter},
    {0x42fed1fd /* contentType */, XFA_Attribute::ContentType},
    {0x453eaf38 /* startNew */, XFA_Attribute::StartNew},
    {0x45a6daf8 /* eofAction */, XFA_Attribute::EofAction},
    {0x47cfa43a /* allowNeutral */, XFA_Attribute::AllowNeutral},
    {0x47d03490 /* connection */, XFA_Attribute::Connection},
    {0x4873c601 /* baselineShift */, XFA_Attribute::BaselineShift},
    {0x4b319767 /* overlinePeriod */, XFA_Attribute::OverlinePeriod},
    {0x4b8bc840 /* fracDigits */, XFA_Attribute::FracDigits},
    {0x4ef3d02c /* orientation */, XFA_Attribute::Orientation},
    {0x4fdc3454 /* timeStamp */, XFA_Attribute::TimeStamp},
    {0x52666f1c /* printCheckDigit */, XFA_Attribute::PrintCheckDigit},
    {0x534729c9 /* marginLeft */, XFA_Attribute::MarginLeft},
    {0x5392ea58 /* stroke */, XFA_Attribute::Stroke},
    {0x5404d6df /* moduleHeight */, XFA_Attribute::ModuleHeight},
    {0x54fa722c /* transferEncoding */, XFA_Attribute::TransferEncoding},
    {0x552d9ad5 /* usage */, XFA_Attribute::Usage},
    {0x570ce835 /* presence */, XFA_Attribute::Presence},
    {0x5739d1ff /* radixOffset */, XFA_Attribute::RadixOffset},
    {0x577682ac /* preserve */, XFA_Attribute::Preserve},
    {0x58be2870 /* aliasNode */, XFA_Attribute::AliasNode},
    {0x5a32e493 /* multiLine */, XFA_Attribute::MultiLine},
    {0x5a50e9e6 /* version */, XFA_Attribute::Version},
    {0x5ab23b6c /* startChar */, XFA_Attribute::StartChar},
    {0x5b707a35 /* scriptTest */, XFA_Attribute::ScriptTest},
    {0x5c054755 /* startAngle */, XFA_Attribute::StartAngle},
    {0x5ec958c0 /* cursorType */, XFA_Attribute::CursorType},
    {0x5f760b50 /* digitalSignature */, XFA_Attribute::DigitalSignature},
    {0x60a61edd /* codeType */, XFA_Attribute::CodeType},
    {0x60d4c8b1 /* output */, XFA_Attribute::Output},
    {0x64110ab5 /* bookendTrailer */, XFA_Attribute::BookendTrailer},
    {0x65e30c67 /* imagingBBox */, XFA_Attribute::ImagingBBox},
    {0x66539c48 /* excludeInitialCap */, XFA_Attribute::ExcludeInitialCap},
    {0x66642f8f /* force */, XFA_Attribute::Force},
    {0x69aa2292 /* crlSign */, XFA_Attribute::CrlSign},
    {0x6a3405dd /* previous */, XFA_Attribute::Previous},
    {0x6a95c976 /* pushCharacterCount */, XFA_Attribute::PushCharacterCount},
    {0x6b6ddcfb /* nullTest */, XFA_Attribute::NullTest},
    {0x6cfa828a /* runAt */, XFA_Attribute::RunAt},
    {0x731e0665 /* spaceBelow */, XFA_Attribute::SpaceBelow},
    {0x74788f8b /* sweepAngle */, XFA_Attribute::SweepAngle},
    {0x78bff531 /* numberOfCells */, XFA_Attribute::NumberOfCells},
    {0x79543055 /* letterSpacing */, XFA_Attribute::LetterSpacing},
    {0x79975f2b /* lockType */, XFA_Attribute::LockType},
    {0x7a0cc471 /* passwordChar */, XFA_Attribute::PasswordChar},
    {0x7a7cc341 /* vAlign */, XFA_Attribute::VAlign},
    {0x7b29630a /* sourceBelow */, XFA_Attribute::SourceBelow},
    {0x7b95e661 /* inverted */, XFA_Attribute::Inverted},
    {0x7c2fd80b /* mark */, XFA_Attribute::Mark},
    {0x7c2ff6ae /* maxH */, XFA_Attribute::MaxH},
    {0x7c2ff6bd /* maxW */, XFA_Attribute::MaxW},
    {0x7c732a66 /* truncate */, XFA_Attribute::Truncate},
    {0x7d02356c /* minH */, XFA_Attribute::MinH},
    {0x7d02357b /* minW */, XFA_Attribute::MinW},
    {0x7d0b5fca /* initial */, XFA_Attribute::Initial},
    {0x7d9fd7c5 /* mode */, XFA_Attribute::Mode},
    {0x7e7e845e /* layout */, XFA_Attribute::Layout},
    {0x7f6fd3d7 /* server */, XFA_Attribute::Server},
    {0x824f21b7 /* embedPDF */, XFA_Attribute::EmbedPDF},
    {0x8340ea66 /* oddOrEven */, XFA_Attribute::OddOrEven},
    {0x836d4d7c /* tabDefault */, XFA_Attribute::TabDefault},
    {0x8855805f /* contains */, XFA_Attribute::Contains},
    {0x8a692521 /* rightInset */, XFA_Attribute::RightInset},
    {0x8af2e657 /* maxChars */, XFA_Attribute::MaxChars},
    {0x8b90e1f2 /* open */, XFA_Attribute::Open},
    {0x8c99377e /* relation */, XFA_Attribute::Relation},
    {0x8d181d61 /* wideNarrowRatio */, XFA_Attribute::WideNarrowRatio},
    {0x8e1c2921 /* relevant */, XFA_Attribute::Relevant},
    {0x8e29d794 /* signatureType */, XFA_Attribute::SignatureType},
    {0x8ec6204c /* lineThroughPeriod */, XFA_Attribute::LineThroughPeriod},
    {0x8ed182d1 /* shape */, XFA_Attribute::Shape},
    {0x8fa01790 /* tabStops */, XFA_Attribute::TabStops},
    {0x8fc36c0a /* outputBelow */, XFA_Attribute::OutputBelow},
    {0x9041d4b0 /* short */, XFA_Attribute::Short},
    {0x907c7719 /* fontVerticalScale */, XFA_Attribute::FontVerticalScale},
    {0x94446dcc /* thickness */, XFA_Attribute::Thickness},
    {0x957fa006 /* commitOn */, XFA_Attribute::CommitOn},
    {0x982bd892 /* remainCharacterCount */,
     XFA_Attribute::RemainCharacterCount},
    {0x98fd4d81 /* keyAgreement */, XFA_Attribute::KeyAgreement},
    {0x99800d7a /* errorCorrectionLevel */,
     XFA_Attribute::ErrorCorrectionLevel},
    {0x9a63da3d /* upsMode */, XFA_Attribute::UpsMode},
    {0x9cc17d75 /* mergeMode */, XFA_Attribute::MergeMode},
    {0x9d833d75 /* circular */, XFA_Attribute::Circular},
    {0x9d8ee204 /* psName */, XFA_Attribute::PsName},
    {0x9dcc3ab3 /* trailer */, XFA_Attribute::Trailer},
    {0xa0933954 /* unicodeRange */, XFA_Attribute::UnicodeRange},
    {0xa1b0d2f5 /* executeType */, XFA_Attribute::ExecuteType},
    {0xa25a883d /* duplexImposition */, XFA_Attribute::DuplexImposition},
    {0xa42ca1b7 /* trayIn */, XFA_Attribute::TrayIn},
    {0xa433f001 /* bindingNode */, XFA_Attribute::BindingNode},
    {0xa5340ff5 /* bofAction */, XFA_Attribute::BofAction},
    {0xa5b410cf /* save */, XFA_Attribute::Save},
    {0xa6118c89 /* targetType */, XFA_Attribute::TargetType},
    {0xa66404cb /* keyEncipherment */, XFA_Attribute::KeyEncipherment},
    {0xa6710262 /* credentialServerPolicy */,
     XFA_Attribute::CredentialServerPolicy},
    {0xa686975b /* size */, XFA_Attribute::Size},
    {0xa85e74f3 /* initialNumber */, XFA_Attribute::InitialNumber},
    {0xabef37e3 /* slope */, XFA_Attribute::Slope},
    {0xabfa6c4f /* cSpace */, XFA_Attribute::CSpace},
    {0xac06e2b0 /* colSpan */, XFA_Attribute::ColSpan},
    {0xadc4c77b /* binding */, XFA_Attribute::Binding},
    {0xaf754613 /* checksum */, XFA_Attribute::Checksum},
    {0xb045fbc5 /* charEncoding */, XFA_Attribute::CharEncoding},
    {0xb0e5485d /* bind */, XFA_Attribute::Bind},
    {0xb12128b7 /* textEntry */, XFA_Attribute::TextEntry},
    {0xb373a862 /* archive */, XFA_Attribute::Archive},
    {0xb598a1f7 /* uuid */, XFA_Attribute::Uuid},
    {0xb5e49bf2 /* posture */, XFA_Attribute::Posture},
    {0xb6b44172 /* after */, XFA_Attribute::After},
    {0xb716467b /* orphans */, XFA_Attribute::Orphans},
    {0xbc0c4695 /* qualifiedName */, XFA_Attribute::QualifiedName},
    {0xbc254332 /* usehref */, XFA_Attribute::Usehref},
    {0xbc8fa350 /* locale */, XFA_Attribute::Locale},
    {0xbd6e1d88 /* weight */, XFA_Attribute::Weight},
    {0xbd96a0e9 /* underlinePeriod */, XFA_Attribute::UnderlinePeriod},
    {0xbde9abda /* data */, XFA_Attribute::Data},
    {0xbe52dfbf /* desc */, XFA_Attribute::Desc},
    {0xbe9ba472 /* numbered */, XFA_Attribute::Numbered},
    {0xc035c6b1 /* dataColumnCount */, XFA_Attribute::DataColumnCount},
    {0xc0ec9fa4 /* overline */, XFA_Attribute::Overline},
    {0xc2ba0923 /* urlPolicy */, XFA_Attribute::UrlPolicy},
    {0xc2bd40fd /* anchorType */, XFA_Attribute::AnchorType},
    {0xc39a88bd /* labelRef */, XFA_Attribute::LabelRef},
    {0xc3c1442f /* bookendLeader */, XFA_Attribute::BookendLeader},
    {0xc4547a08 /* maxLength */, XFA_Attribute::MaxLength},
    {0xc4fed09b /* accessKey */, XFA_Attribute::AccessKey},
    {0xc5762157 /* cursorLocation */, XFA_Attribute::CursorLocation},
    {0xc860f30a /* delayedOpen */, XFA_Attribute::DelayedOpen},
    {0xc8da4da7 /* target */, XFA_Attribute::Target},
    {0xca5dc27c /* dataEncipherment */, XFA_Attribute::DataEncipherment},
    {0xcb150479 /* afterTarget */, XFA_Attribute::AfterTarget},
    {0xcbcaf66d /* leader */, XFA_Attribute::Leader},
    {0xcca7897e /* picker */, XFA_Attribute::Picker},
    {0xcd7f7b54 /* from */, XFA_Attribute::From},
    {0xcea5e62c /* baseProfile */, XFA_Attribute::BaseProfile},
    {0xd171b240 /* aspect */, XFA_Attribute::Aspect},
    {0xd3c84d25 /* rowColumnRatio */, XFA_Attribute::RowColumnRatio},
    {0xd4b01921 /* lineHeight */, XFA_Attribute::LineHeight},
    {0xd4cc53f8 /* highlight */, XFA_Attribute::Highlight},
    {0xd50f903a /* valueRef */, XFA_Attribute::ValueRef},
    {0xd52482e0 /* maxEntries */, XFA_Attribute::MaxEntries},
    {0xd57c513c /* dataLength */, XFA_Attribute::DataLength},
    {0xd6128d8d /* activity */, XFA_Attribute::Activity},
    {0xd6a39990 /* input */, XFA_Attribute::Input},
    {0xd6e27f1d /* value */, XFA_Attribute::Value},
    {0xd70798c2 /* blankOrNotBlank */, XFA_Attribute::BlankOrNotBlank},
    {0xd861f8af /* addRevocationInfo */, XFA_Attribute::AddRevocationInfo},
    {0xd8f982bf /* genericFamily */, XFA_Attribute::GenericFamily},
    {0xd996fa9b /* hand */, XFA_Attribute::Hand},
    {0xdb55fec5 /* href */, XFA_Attribute::Href},
    {0xdc75676c /* textEncoding */, XFA_Attribute::TextEncoding},
    {0xde7f92ba /* leadDigits */, XFA_Attribute::LeadDigits},
    {0xe11a2cbc /* permissions */, XFA_Attribute::Permissions},
    {0xe18b5659 /* spaceAbove */, XFA_Attribute::SpaceAbove},
    {0xe1a26b56 /* codeBase */, XFA_Attribute::CodeBase},
    {0xe349d044 /* stock */, XFA_Attribute::Stock},
    {0xe372ae97 /* isNull */, XFA_Attribute::IsNull},
    {0xe4c3a5e5 /* restoreState */, XFA_Attribute::RestoreState},
    {0xe5c96d6a /* excludeAllCaps */, XFA_Attribute::ExcludeAllCaps},
    {0xe64b1129 /* formatTest */, XFA_Attribute::FormatTest},
    {0xe6f99487 /* hScrollPolicy */, XFA_Attribute::HScrollPolicy},
    {0xe8dddf50 /* join */, XFA_Attribute::Join},
    {0xe8f118a8 /* keyCertSign */, XFA_Attribute::KeyCertSign},
    {0xe948b9a8 /* radius */, XFA_Attribute::Radius},
    {0xe996b2fe /* sourceAbove */, XFA_Attribute::SourceAbove},
    {0xea7090a0 /* override */, XFA_Attribute::Override},
    {0xeb091003 /* classId */, XFA_Attribute::ClassId},
    {0xeb511b54 /* disable */, XFA_Attribute::Disable},
    {0xeda9017a /* scope */, XFA_Attribute::Scope},
    {0xf197844d /* match */, XFA_Attribute::Match},
    {0xf2009339 /* placement */, XFA_Attribute::Placement},
    {0xf4ffce73 /* before */, XFA_Attribute::Before},
    {0xf531b059 /* writingScript */, XFA_Attribute::WritingScript},
    {0xf575ca75 /* endChar */, XFA_Attribute::EndChar},
    {0xf6b47749 /* lock */, XFA_Attribute::Lock},
    {0xf6b4afb0 /* long */, XFA_Attribute::Long},
    {0xf6b59543 /* intact */, XFA_Attribute::Intact},
    {0xf889e747 /* xdpContent */, XFA_Attribute::XdpContent},
    {0xfea53ec6 /* decipherOnly */, XFA_Attribute::DecipherOnly},
};

}  // namespace

// static
XFA_Element CXFA_Node::NameToElement(const WideString& name) {
  uint32_t hash = FX_HashCode_GetW(name.AsStringView(), false);
  auto* elem = std::lower_bound(
      std::begin(ElementNameToEnum), std::end(ElementNameToEnum), hash,
      [](const ElementNameInfo& a, uint32_t hash) { return a.hash < hash; });
  if (elem != std::end(ElementNameToEnum) && elem->hash == hash)
    return elem->element;
  return XFA_Element::Unknown;
}

// static
XFA_Attribute CXFA_Node::NameToAttribute(const WideStringView& name) {
  uint32_t hash = FX_HashCode_GetW(name, false);
  auto* elem = std::lower_bound(
      std::begin(AttributeNameInfoToEnum), std::end(AttributeNameInfoToEnum),
      hash,
      [](const AttributeNameInfo& a, uint32_t hash) { return a.hash < hash; });
  if (elem != std::end(AttributeNameInfoToEnum) && elem->hash == hash)
    return elem->attribute;
  return XFA_Attribute::Unknown;
}

// static
std::unique_ptr<CXFA_Node> CXFA_Node::Create(CXFA_Document* doc,
                                             XFA_Element element,
                                             XFA_PacketType packet) {
  std::unique_ptr<CXFA_Node> node;
  switch (element) {
    case XFA_Element::Ps:
      node = pdfium::MakeUnique<CXFA_Ps>(doc, packet);
      break;
    case XFA_Element::To:
      node = pdfium::MakeUnique<CXFA_To>(doc, packet);
      break;
    case XFA_Element::Ui:
      node = pdfium::MakeUnique<CXFA_Ui>(doc, packet);
      break;
    case XFA_Element::RecordSet:
      node = pdfium::MakeUnique<CXFA_RecordSet>(doc, packet);
      break;
    case XFA_Element::SubsetBelow:
      node = pdfium::MakeUnique<CXFA_SubsetBelow>(doc, packet);
      break;
    case XFA_Element::SubformSet:
      node = pdfium::MakeUnique<CXFA_SubformSet>(doc, packet);
      break;
    case XFA_Element::AdobeExtensionLevel:
      node = pdfium::MakeUnique<CXFA_AdobeExtensionLevel>(doc, packet);
      break;
    case XFA_Element::Typeface:
      node = pdfium::MakeUnique<CXFA_Typeface>(doc, packet);
      break;
    case XFA_Element::Break:
      node = pdfium::MakeUnique<CXFA_Break>(doc, packet);
      break;
    case XFA_Element::FontInfo:
      node = pdfium::MakeUnique<CXFA_FontInfo>(doc, packet);
      break;
    case XFA_Element::NumberPattern:
      node = pdfium::MakeUnique<CXFA_NumberPattern>(doc, packet);
      break;
    case XFA_Element::DynamicRender:
      node = pdfium::MakeUnique<CXFA_DynamicRender>(doc, packet);
      break;
    case XFA_Element::PrintScaling:
      node = pdfium::MakeUnique<CXFA_PrintScaling>(doc, packet);
      break;
    case XFA_Element::CheckButton:
      node = pdfium::MakeUnique<CXFA_CheckButton>(doc, packet);
      break;
    case XFA_Element::DatePatterns:
      node = pdfium::MakeUnique<CXFA_DatePatterns>(doc, packet);
      break;
    case XFA_Element::SourceSet:
      node = pdfium::MakeUnique<CXFA_SourceSet>(doc, packet);
      break;
    case XFA_Element::Amd:
      node = pdfium::MakeUnique<CXFA_Amd>(doc, packet);
      break;
    case XFA_Element::Arc:
      node = pdfium::MakeUnique<CXFA_Arc>(doc, packet);
      break;
    case XFA_Element::Day:
      node = pdfium::MakeUnique<CXFA_Day>(doc, packet);
      break;
    case XFA_Element::Era:
      node = pdfium::MakeUnique<CXFA_Era>(doc, packet);
      break;
    case XFA_Element::Jog:
      node = pdfium::MakeUnique<CXFA_Jog>(doc, packet);
      break;
    case XFA_Element::Log:
      node = pdfium::MakeUnique<CXFA_Log>(doc, packet);
      break;
    case XFA_Element::Map:
      node = pdfium::MakeUnique<CXFA_Map>(doc, packet);
      break;
    case XFA_Element::Mdp:
      node = pdfium::MakeUnique<CXFA_Mdp>(doc, packet);
      break;
    case XFA_Element::BreakBefore:
      node = pdfium::MakeUnique<CXFA_BreakBefore>(doc, packet);
      break;
    case XFA_Element::Oid:
      node = pdfium::MakeUnique<CXFA_Oid>(doc, packet);
      break;
    case XFA_Element::Pcl:
      node = pdfium::MakeUnique<CXFA_Pcl>(doc, packet);
      break;
    case XFA_Element::Pdf:
      node = pdfium::MakeUnique<CXFA_Pdf>(doc, packet);
      break;
    case XFA_Element::Ref:
      node = pdfium::MakeUnique<CXFA_Ref>(doc, packet);
      break;
    case XFA_Element::Uri:
      node = pdfium::MakeUnique<CXFA_Uri>(doc, packet);
      break;
    case XFA_Element::Xdc:
      node = pdfium::MakeUnique<CXFA_Xdc>(doc, packet);
      break;
    case XFA_Element::Xdp:
      node = pdfium::MakeUnique<CXFA_Xdp>(doc, packet);
      break;
    case XFA_Element::Xfa:
      node = pdfium::MakeUnique<CXFA_Xfa>(doc, packet);
      break;
    case XFA_Element::Xsl:
      node = pdfium::MakeUnique<CXFA_Xsl>(doc, packet);
      break;
    case XFA_Element::Zpl:
      node = pdfium::MakeUnique<CXFA_Zpl>(doc, packet);
      break;
    case XFA_Element::Cache:
      node = pdfium::MakeUnique<CXFA_Cache>(doc, packet);
      break;
    case XFA_Element::Margin:
      node = pdfium::MakeUnique<CXFA_Margin>(doc, packet);
      break;
    case XFA_Element::KeyUsage:
      node = pdfium::MakeUnique<CXFA_KeyUsage>(doc, packet);
      break;
    case XFA_Element::Exclude:
      node = pdfium::MakeUnique<CXFA_Exclude>(doc, packet);
      break;
    case XFA_Element::ChoiceList:
      node = pdfium::MakeUnique<CXFA_ChoiceList>(doc, packet);
      break;
    case XFA_Element::Level:
      node = pdfium::MakeUnique<CXFA_Level>(doc, packet);
      break;
    case XFA_Element::LabelPrinter:
      node = pdfium::MakeUnique<CXFA_LabelPrinter>(doc, packet);
      break;
    case XFA_Element::CalendarSymbols:
      node = pdfium::MakeUnique<CXFA_CalendarSymbols>(doc, packet);
      break;
    case XFA_Element::Para:
      node = pdfium::MakeUnique<CXFA_Para>(doc, packet);
      break;
    case XFA_Element::Part:
      node = pdfium::MakeUnique<CXFA_Part>(doc, packet);
      break;
    case XFA_Element::Pdfa:
      node = pdfium::MakeUnique<CXFA_Pdfa>(doc, packet);
      break;
    case XFA_Element::Filter:
      node = pdfium::MakeUnique<CXFA_Filter>(doc, packet);
      break;
    case XFA_Element::Present:
      node = pdfium::MakeUnique<CXFA_Present>(doc, packet);
      break;
    case XFA_Element::Pagination:
      node = pdfium::MakeUnique<CXFA_Pagination>(doc, packet);
      break;
    case XFA_Element::Encoding:
      node = pdfium::MakeUnique<CXFA_Encoding>(doc, packet);
      break;
    case XFA_Element::Event:
      node = pdfium::MakeUnique<CXFA_Event>(doc, packet);
      break;
    case XFA_Element::Whitespace:
      node = pdfium::MakeUnique<CXFA_Whitespace>(doc, packet);
      break;
    case XFA_Element::DefaultUi:
      node = pdfium::MakeUnique<CXFA_DefaultUi>(doc, packet);
      break;
    case XFA_Element::DataModel:
      node = pdfium::MakeUnique<CXFA_DataModel>(doc, packet);
      break;
    case XFA_Element::Barcode:
      node = pdfium::MakeUnique<CXFA_Barcode>(doc, packet);
      break;
    case XFA_Element::TimePattern:
      node = pdfium::MakeUnique<CXFA_TimePattern>(doc, packet);
      break;
    case XFA_Element::BatchOutput:
      node = pdfium::MakeUnique<CXFA_BatchOutput>(doc, packet);
      break;
    case XFA_Element::Enforce:
      node = pdfium::MakeUnique<CXFA_Enforce>(doc, packet);
      break;
    case XFA_Element::CurrencySymbols:
      node = pdfium::MakeUnique<CXFA_CurrencySymbols>(doc, packet);
      break;
    case XFA_Element::AddSilentPrint:
      node = pdfium::MakeUnique<CXFA_AddSilentPrint>(doc, packet);
      break;
    case XFA_Element::Rename:
      node = pdfium::MakeUnique<CXFA_Rename>(doc, packet);
      break;
    case XFA_Element::Operation:
      node = pdfium::MakeUnique<CXFA_Operation>(doc, packet);
      break;
    case XFA_Element::Typefaces:
      node = pdfium::MakeUnique<CXFA_Typefaces>(doc, packet);
      break;
    case XFA_Element::SubjectDNs:
      node = pdfium::MakeUnique<CXFA_SubjectDNs>(doc, packet);
      break;
    case XFA_Element::Issuers:
      node = pdfium::MakeUnique<CXFA_Issuers>(doc, packet);
      break;
    case XFA_Element::WsdlConnection:
      node = pdfium::MakeUnique<CXFA_WsdlConnection>(doc, packet);
      break;
    case XFA_Element::Debug:
      node = pdfium::MakeUnique<CXFA_Debug>(doc, packet);
      break;
    case XFA_Element::Delta:
      node = pdfium::MakeUnique<CXFA_Delta>(doc, packet);
      break;
    case XFA_Element::EraNames:
      node = pdfium::MakeUnique<CXFA_EraNames>(doc, packet);
      break;
    case XFA_Element::ModifyAnnots:
      node = pdfium::MakeUnique<CXFA_ModifyAnnots>(doc, packet);
      break;
    case XFA_Element::StartNode:
      node = pdfium::MakeUnique<CXFA_StartNode>(doc, packet);
      break;
    case XFA_Element::Button:
      node = pdfium::MakeUnique<CXFA_Button>(doc, packet);
      break;
    case XFA_Element::Format:
      node = pdfium::MakeUnique<CXFA_Format>(doc, packet);
      break;
    case XFA_Element::Border:
      node = pdfium::MakeUnique<CXFA_Border>(doc, packet);
      break;
    case XFA_Element::Area:
      node = pdfium::MakeUnique<CXFA_Area>(doc, packet);
      break;
    case XFA_Element::Hyphenation:
      node = pdfium::MakeUnique<CXFA_Hyphenation>(doc, packet);
      break;
    case XFA_Element::Text:
      node = pdfium::MakeUnique<CXFA_Text>(doc, packet);
      break;
    case XFA_Element::Time:
      node = pdfium::MakeUnique<CXFA_Time>(doc, packet);
      break;
    case XFA_Element::Type:
      node = pdfium::MakeUnique<CXFA_Type>(doc, packet);
      break;
    case XFA_Element::Overprint:
      node = pdfium::MakeUnique<CXFA_Overprint>(doc, packet);
      break;
    case XFA_Element::Certificates:
      node = pdfium::MakeUnique<CXFA_Certificates>(doc, packet);
      break;
    case XFA_Element::EncryptionMethods:
      node = pdfium::MakeUnique<CXFA_EncryptionMethods>(doc, packet);
      break;
    case XFA_Element::SetProperty:
      node = pdfium::MakeUnique<CXFA_SetProperty>(doc, packet);
      break;
    case XFA_Element::PrinterName:
      node = pdfium::MakeUnique<CXFA_PrinterName>(doc, packet);
      break;
    case XFA_Element::StartPage:
      node = pdfium::MakeUnique<CXFA_StartPage>(doc, packet);
      break;
    case XFA_Element::PageOffset:
      node = pdfium::MakeUnique<CXFA_PageOffset>(doc, packet);
      break;
    case XFA_Element::DateTime:
      node = pdfium::MakeUnique<CXFA_DateTime>(doc, packet);
      break;
    case XFA_Element::Comb:
      node = pdfium::MakeUnique<CXFA_Comb>(doc, packet);
      break;
    case XFA_Element::Pattern:
      node = pdfium::MakeUnique<CXFA_Pattern>(doc, packet);
      break;
    case XFA_Element::IfEmpty:
      node = pdfium::MakeUnique<CXFA_IfEmpty>(doc, packet);
      break;
    case XFA_Element::SuppressBanner:
      node = pdfium::MakeUnique<CXFA_SuppressBanner>(doc, packet);
      break;
    case XFA_Element::OutputBin:
      node = pdfium::MakeUnique<CXFA_OutputBin>(doc, packet);
      break;
    case XFA_Element::Field:
      node = pdfium::MakeUnique<CXFA_Field>(doc, packet);
      break;
    case XFA_Element::Agent:
      node = pdfium::MakeUnique<CXFA_Agent>(doc, packet);
      break;
    case XFA_Element::OutputXSL:
      node = pdfium::MakeUnique<CXFA_OutputXSL>(doc, packet);
      break;
    case XFA_Element::AdjustData:
      node = pdfium::MakeUnique<CXFA_AdjustData>(doc, packet);
      break;
    case XFA_Element::AutoSave:
      node = pdfium::MakeUnique<CXFA_AutoSave>(doc, packet);
      break;
    case XFA_Element::ContentArea:
      node = pdfium::MakeUnique<CXFA_ContentArea>(doc, packet);
      break;
    case XFA_Element::WsdlAddress:
      node = pdfium::MakeUnique<CXFA_WsdlAddress>(doc, packet);
      break;
    case XFA_Element::Solid:
      node = pdfium::MakeUnique<CXFA_Solid>(doc, packet);
      break;
    case XFA_Element::DateTimeSymbols:
      node = pdfium::MakeUnique<CXFA_DateTimeSymbols>(doc, packet);
      break;
    case XFA_Element::EncryptionLevel:
      node = pdfium::MakeUnique<CXFA_EncryptionLevel>(doc, packet);
      break;
    case XFA_Element::Edge:
      node = pdfium::MakeUnique<CXFA_Edge>(doc, packet);
      break;
    case XFA_Element::Stipple:
      node = pdfium::MakeUnique<CXFA_Stipple>(doc, packet);
      break;
    case XFA_Element::Attributes:
      node = pdfium::MakeUnique<CXFA_Attributes>(doc, packet);
      break;
    case XFA_Element::VersionControl:
      node = pdfium::MakeUnique<CXFA_VersionControl>(doc, packet);
      break;
    case XFA_Element::Meridiem:
      node = pdfium::MakeUnique<CXFA_Meridiem>(doc, packet);
      break;
    case XFA_Element::ExclGroup:
      node = pdfium::MakeUnique<CXFA_ExclGroup>(doc, packet);
      break;
    case XFA_Element::ToolTip:
      node = pdfium::MakeUnique<CXFA_ToolTip>(doc, packet);
      break;
    case XFA_Element::Compress:
      node = pdfium::MakeUnique<CXFA_Compress>(doc, packet);
      break;
    case XFA_Element::Reason:
      node = pdfium::MakeUnique<CXFA_Reason>(doc, packet);
      break;
    case XFA_Element::Execute:
      node = pdfium::MakeUnique<CXFA_Execute>(doc, packet);
      break;
    case XFA_Element::ContentCopy:
      node = pdfium::MakeUnique<CXFA_ContentCopy>(doc, packet);
      break;
    case XFA_Element::DateTimeEdit:
      node = pdfium::MakeUnique<CXFA_DateTimeEdit>(doc, packet);
      break;
    case XFA_Element::Config:
      node = pdfium::MakeUnique<CXFA_Config>(doc, packet);
      break;
    case XFA_Element::Image:
      node = pdfium::MakeUnique<CXFA_Image>(doc, packet);
      break;
    case XFA_Element::SharpxHTML:
      node = pdfium::MakeUnique<CXFA_SharpxHTML>(doc, packet);
      break;
    case XFA_Element::NumberOfCopies:
      node = pdfium::MakeUnique<CXFA_NumberOfCopies>(doc, packet);
      break;
    case XFA_Element::BehaviorOverride:
      node = pdfium::MakeUnique<CXFA_BehaviorOverride>(doc, packet);
      break;
    case XFA_Element::TimeStamp:
      node = pdfium::MakeUnique<CXFA_TimeStamp>(doc, packet);
      break;
    case XFA_Element::Month:
      node = pdfium::MakeUnique<CXFA_Month>(doc, packet);
      break;
    case XFA_Element::ViewerPreferences:
      node = pdfium::MakeUnique<CXFA_ViewerPreferences>(doc, packet);
      break;
    case XFA_Element::ScriptModel:
      node = pdfium::MakeUnique<CXFA_ScriptModel>(doc, packet);
      break;
    case XFA_Element::Decimal:
      node = pdfium::MakeUnique<CXFA_Decimal>(doc, packet);
      break;
    case XFA_Element::Subform:
      node = pdfium::MakeUnique<CXFA_Subform>(doc, packet);
      break;
    case XFA_Element::Select:
      node = pdfium::MakeUnique<CXFA_Select>(doc, packet);
      break;
    case XFA_Element::Window:
      node = pdfium::MakeUnique<CXFA_Window>(doc, packet);
      break;
    case XFA_Element::LocaleSet:
      node = pdfium::MakeUnique<CXFA_LocaleSet>(doc, packet);
      break;
    case XFA_Element::Handler:
      node = pdfium::MakeUnique<CXFA_Handler>(doc, packet);
      break;
    case XFA_Element::Presence:
      node = pdfium::MakeUnique<CXFA_Presence>(doc, packet);
      break;
    case XFA_Element::Record:
      node = pdfium::MakeUnique<CXFA_Record>(doc, packet);
      break;
    case XFA_Element::Embed:
      node = pdfium::MakeUnique<CXFA_Embed>(doc, packet);
      break;
    case XFA_Element::Version:
      node = pdfium::MakeUnique<CXFA_Version>(doc, packet);
      break;
    case XFA_Element::Command:
      node = pdfium::MakeUnique<CXFA_Command>(doc, packet);
      break;
    case XFA_Element::Copies:
      node = pdfium::MakeUnique<CXFA_Copies>(doc, packet);
      break;
    case XFA_Element::Staple:
      node = pdfium::MakeUnique<CXFA_Staple>(doc, packet);
      break;
    case XFA_Element::SubmitFormat:
      node = pdfium::MakeUnique<CXFA_SubmitFormat>(doc, packet);
      break;
    case XFA_Element::Boolean:
      node = pdfium::MakeUnique<CXFA_Boolean>(doc, packet);
      break;
    case XFA_Element::Message:
      node = pdfium::MakeUnique<CXFA_Message>(doc, packet);
      break;
    case XFA_Element::Output:
      node = pdfium::MakeUnique<CXFA_Output>(doc, packet);
      break;
    case XFA_Element::PsMap:
      node = pdfium::MakeUnique<CXFA_PsMap>(doc, packet);
      break;
    case XFA_Element::ExcludeNS:
      node = pdfium::MakeUnique<CXFA_ExcludeNS>(doc, packet);
      break;
    case XFA_Element::Assist:
      node = pdfium::MakeUnique<CXFA_Assist>(doc, packet);
      break;
    case XFA_Element::Picture:
      node = pdfium::MakeUnique<CXFA_Picture>(doc, packet);
      break;
    case XFA_Element::Traversal:
      node = pdfium::MakeUnique<CXFA_Traversal>(doc, packet);
      break;
    case XFA_Element::SilentPrint:
      node = pdfium::MakeUnique<CXFA_SilentPrint>(doc, packet);
      break;
    case XFA_Element::WebClient:
      node = pdfium::MakeUnique<CXFA_WebClient>(doc, packet);
      break;
    case XFA_Element::Producer:
      node = pdfium::MakeUnique<CXFA_Producer>(doc, packet);
      break;
    case XFA_Element::Corner:
      node = pdfium::MakeUnique<CXFA_Corner>(doc, packet);
      break;
    case XFA_Element::MsgId:
      node = pdfium::MakeUnique<CXFA_MsgId>(doc, packet);
      break;
    case XFA_Element::Color:
      node = pdfium::MakeUnique<CXFA_Color>(doc, packet);
      break;
    case XFA_Element::Keep:
      node = pdfium::MakeUnique<CXFA_Keep>(doc, packet);
      break;
    case XFA_Element::Query:
      node = pdfium::MakeUnique<CXFA_Query>(doc, packet);
      break;
    case XFA_Element::Insert:
      node = pdfium::MakeUnique<CXFA_Insert>(doc, packet);
      break;
    case XFA_Element::ImageEdit:
      node = pdfium::MakeUnique<CXFA_ImageEdit>(doc, packet);
      break;
    case XFA_Element::Validate:
      node = pdfium::MakeUnique<CXFA_Validate>(doc, packet);
      break;
    case XFA_Element::DigestMethods:
      node = pdfium::MakeUnique<CXFA_DigestMethods>(doc, packet);
      break;
    case XFA_Element::NumberPatterns:
      node = pdfium::MakeUnique<CXFA_NumberPatterns>(doc, packet);
      break;
    case XFA_Element::PageSet:
      node = pdfium::MakeUnique<CXFA_PageSet>(doc, packet);
      break;
    case XFA_Element::Integer:
      node = pdfium::MakeUnique<CXFA_Integer>(doc, packet);
      break;
    case XFA_Element::SoapAddress:
      node = pdfium::MakeUnique<CXFA_SoapAddress>(doc, packet);
      break;
    case XFA_Element::Equate:
      node = pdfium::MakeUnique<CXFA_Equate>(doc, packet);
      break;
    case XFA_Element::FormFieldFilling:
      node = pdfium::MakeUnique<CXFA_FormFieldFilling>(doc, packet);
      break;
    case XFA_Element::PageRange:
      node = pdfium::MakeUnique<CXFA_PageRange>(doc, packet);
      break;
    case XFA_Element::Update:
      node = pdfium::MakeUnique<CXFA_Update>(doc, packet);
      break;
    case XFA_Element::ConnectString:
      node = pdfium::MakeUnique<CXFA_ConnectString>(doc, packet);
      break;
    case XFA_Element::Mode:
      node = pdfium::MakeUnique<CXFA_Mode>(doc, packet);
      break;
    case XFA_Element::Layout:
      node = pdfium::MakeUnique<CXFA_Layout>(doc, packet);
      break;
    case XFA_Element::Sharpxml:
      node = pdfium::MakeUnique<CXFA_Sharpxml>(doc, packet);
      break;
    case XFA_Element::XsdConnection:
      node = pdfium::MakeUnique<CXFA_XsdConnection>(doc, packet);
      break;
    case XFA_Element::Traverse:
      node = pdfium::MakeUnique<CXFA_Traverse>(doc, packet);
      break;
    case XFA_Element::Encodings:
      node = pdfium::MakeUnique<CXFA_Encodings>(doc, packet);
      break;
    case XFA_Element::Template:
      node = pdfium::MakeUnique<CXFA_Template>(doc, packet);
      break;
    case XFA_Element::Acrobat:
      node = pdfium::MakeUnique<CXFA_Acrobat>(doc, packet);
      break;
    case XFA_Element::ValidationMessaging:
      node = pdfium::MakeUnique<CXFA_ValidationMessaging>(doc, packet);
      break;
    case XFA_Element::Signing:
      node = pdfium::MakeUnique<CXFA_Signing>(doc, packet);
      break;
    case XFA_Element::Script:
      node = pdfium::MakeUnique<CXFA_Script>(doc, packet);
      break;
    case XFA_Element::AddViewerPreferences:
      node = pdfium::MakeUnique<CXFA_AddViewerPreferences>(doc, packet);
      break;
    case XFA_Element::AlwaysEmbed:
      node = pdfium::MakeUnique<CXFA_AlwaysEmbed>(doc, packet);
      break;
    case XFA_Element::PasswordEdit:
      node = pdfium::MakeUnique<CXFA_PasswordEdit>(doc, packet);
      break;
    case XFA_Element::NumericEdit:
      node = pdfium::MakeUnique<CXFA_NumericEdit>(doc, packet);
      break;
    case XFA_Element::EncryptionMethod:
      node = pdfium::MakeUnique<CXFA_EncryptionMethod>(doc, packet);
      break;
    case XFA_Element::Change:
      node = pdfium::MakeUnique<CXFA_Change>(doc, packet);
      break;
    case XFA_Element::PageArea:
      node = pdfium::MakeUnique<CXFA_PageArea>(doc, packet);
      break;
    case XFA_Element::SubmitUrl:
      node = pdfium::MakeUnique<CXFA_SubmitUrl>(doc, packet);
      break;
    case XFA_Element::Oids:
      node = pdfium::MakeUnique<CXFA_Oids>(doc, packet);
      break;
    case XFA_Element::Signature:
      node = pdfium::MakeUnique<CXFA_Signature>(doc, packet);
      break;
    case XFA_Element::ADBE_JSConsole:
      node = pdfium::MakeUnique<CXFA_aDBE_JSConsole>(doc, packet);
      break;
    case XFA_Element::Caption:
      node = pdfium::MakeUnique<CXFA_Caption>(doc, packet);
      break;
    case XFA_Element::Relevant:
      node = pdfium::MakeUnique<CXFA_Relevant>(doc, packet);
      break;
    case XFA_Element::FlipLabel:
      node = pdfium::MakeUnique<CXFA_FlipLabel>(doc, packet);
      break;
    case XFA_Element::ExData:
      node = pdfium::MakeUnique<CXFA_ExData>(doc, packet);
      break;
    case XFA_Element::DayNames:
      node = pdfium::MakeUnique<CXFA_DayNames>(doc, packet);
      break;
    case XFA_Element::SoapAction:
      node = pdfium::MakeUnique<CXFA_SoapAction>(doc, packet);
      break;
    case XFA_Element::DefaultTypeface:
      node = pdfium::MakeUnique<CXFA_DefaultTypeface>(doc, packet);
      break;
    case XFA_Element::Manifest:
      node = pdfium::MakeUnique<CXFA_Manifest>(doc, packet);
      break;
    case XFA_Element::Overflow:
      node = pdfium::MakeUnique<CXFA_Overflow>(doc, packet);
      break;
    case XFA_Element::Linear:
      node = pdfium::MakeUnique<CXFA_Linear>(doc, packet);
      break;
    case XFA_Element::CurrencySymbol:
      node = pdfium::MakeUnique<CXFA_CurrencySymbol>(doc, packet);
      break;
    case XFA_Element::Delete:
      node = pdfium::MakeUnique<CXFA_Delete>(doc, packet);
      break;
    case XFA_Element::DigestMethod:
      node = pdfium::MakeUnique<CXFA_DigestMethod>(doc, packet);
      break;
    case XFA_Element::InstanceManager:
      node = pdfium::MakeUnique<CXFA_InstanceManager>(doc, packet);
      break;
    case XFA_Element::EquateRange:
      node = pdfium::MakeUnique<CXFA_EquateRange>(doc, packet);
      break;
    case XFA_Element::Medium:
      node = pdfium::MakeUnique<CXFA_Medium>(doc, packet);
      break;
    case XFA_Element::TextEdit:
      node = pdfium::MakeUnique<CXFA_TextEdit>(doc, packet);
      break;
    case XFA_Element::TemplateCache:
      node = pdfium::MakeUnique<CXFA_TemplateCache>(doc, packet);
      break;
    case XFA_Element::CompressObjectStream:
      node = pdfium::MakeUnique<CXFA_CompressObjectStream>(doc, packet);
      break;
    case XFA_Element::DataValue:
      node = pdfium::MakeUnique<CXFA_DataValue>(doc, packet);
      break;
    case XFA_Element::AccessibleContent:
      node = pdfium::MakeUnique<CXFA_AccessibleContent>(doc, packet);
      break;
    case XFA_Element::IncludeXDPContent:
      node = pdfium::MakeUnique<CXFA_IncludeXDPContent>(doc, packet);
      break;
    case XFA_Element::XmlConnection:
      node = pdfium::MakeUnique<CXFA_XmlConnection>(doc, packet);
      break;
    case XFA_Element::ValidateApprovalSignatures:
      node = pdfium::MakeUnique<CXFA_ValidateApprovalSignatures>(doc, packet);
      break;
    case XFA_Element::SignData:
      node = pdfium::MakeUnique<CXFA_SignData>(doc, packet);
      break;
    case XFA_Element::Packets:
      node = pdfium::MakeUnique<CXFA_Packets>(doc, packet);
      break;
    case XFA_Element::DatePattern:
      node = pdfium::MakeUnique<CXFA_DatePattern>(doc, packet);
      break;
    case XFA_Element::DuplexOption:
      node = pdfium::MakeUnique<CXFA_DuplexOption>(doc, packet);
      break;
    case XFA_Element::Base:
      node = pdfium::MakeUnique<CXFA_Base>(doc, packet);
      break;
    case XFA_Element::Bind:
      node = pdfium::MakeUnique<CXFA_Bind>(doc, packet);
      break;
    case XFA_Element::Compression:
      node = pdfium::MakeUnique<CXFA_Compression>(doc, packet);
      break;
    case XFA_Element::User:
      node = pdfium::MakeUnique<CXFA_User>(doc, packet);
      break;
    case XFA_Element::Rectangle:
      node = pdfium::MakeUnique<CXFA_Rectangle>(doc, packet);
      break;
    case XFA_Element::EffectiveOutputPolicy:
      node = pdfium::MakeUnique<CXFA_EffectiveOutputPolicy>(doc, packet);
      break;
    case XFA_Element::ADBE_JSDebugger:
      node = pdfium::MakeUnique<CXFA_aDBE_JSDebugger>(doc, packet);
      break;
    case XFA_Element::Acrobat7:
      node = pdfium::MakeUnique<CXFA_Acrobat7>(doc, packet);
      break;
    case XFA_Element::Interactive:
      node = pdfium::MakeUnique<CXFA_Interactive>(doc, packet);
      break;
    case XFA_Element::Locale:
      node = pdfium::MakeUnique<CXFA_Locale>(doc, packet);
      break;
    case XFA_Element::CurrentPage:
      node = pdfium::MakeUnique<CXFA_CurrentPage>(doc, packet);
      break;
    case XFA_Element::Data:
      node = pdfium::MakeUnique<CXFA_Data>(doc, packet);
      break;
    case XFA_Element::Date:
      node = pdfium::MakeUnique<CXFA_Date>(doc, packet);
      break;
    case XFA_Element::Desc:
      node = pdfium::MakeUnique<CXFA_Desc>(doc, packet);
      break;
    case XFA_Element::Encrypt:
      node = pdfium::MakeUnique<CXFA_Encrypt>(doc, packet);
      break;
    case XFA_Element::Draw:
      node = pdfium::MakeUnique<CXFA_Draw>(doc, packet);
      break;
    case XFA_Element::Encryption:
      node = pdfium::MakeUnique<CXFA_Encryption>(doc, packet);
      break;
    case XFA_Element::MeridiemNames:
      node = pdfium::MakeUnique<CXFA_MeridiemNames>(doc, packet);
      break;
    case XFA_Element::Messaging:
      node = pdfium::MakeUnique<CXFA_Messaging>(doc, packet);
      break;
    case XFA_Element::Speak:
      node = pdfium::MakeUnique<CXFA_Speak>(doc, packet);
      break;
    case XFA_Element::DataGroup:
      node = pdfium::MakeUnique<CXFA_DataGroup>(doc, packet);
      break;
    case XFA_Element::Common:
      node = pdfium::MakeUnique<CXFA_Common>(doc, packet);
      break;
    case XFA_Element::Sharptext:
      node = pdfium::MakeUnique<CXFA_Sharptext>(doc, packet);
      break;
    case XFA_Element::PaginationOverride:
      node = pdfium::MakeUnique<CXFA_PaginationOverride>(doc, packet);
      break;
    case XFA_Element::Reasons:
      node = pdfium::MakeUnique<CXFA_Reasons>(doc, packet);
      break;
    case XFA_Element::SignatureProperties:
      node = pdfium::MakeUnique<CXFA_SignatureProperties>(doc, packet);
      break;
    case XFA_Element::Threshold:
      node = pdfium::MakeUnique<CXFA_Threshold>(doc, packet);
      break;
    case XFA_Element::AppearanceFilter:
      node = pdfium::MakeUnique<CXFA_AppearanceFilter>(doc, packet);
      break;
    case XFA_Element::Fill:
      node = pdfium::MakeUnique<CXFA_Fill>(doc, packet);
      break;
    case XFA_Element::Font:
      node = pdfium::MakeUnique<CXFA_Font>(doc, packet);
      break;
    case XFA_Element::Form:
      node = pdfium::MakeUnique<CXFA_Form>(doc, packet);
      break;
    case XFA_Element::MediumInfo:
      node = pdfium::MakeUnique<CXFA_MediumInfo>(doc, packet);
      break;
    case XFA_Element::Certificate:
      node = pdfium::MakeUnique<CXFA_Certificate>(doc, packet);
      break;
    case XFA_Element::Password:
      node = pdfium::MakeUnique<CXFA_Password>(doc, packet);
      break;
    case XFA_Element::RunScripts:
      node = pdfium::MakeUnique<CXFA_RunScripts>(doc, packet);
      break;
    case XFA_Element::Trace:
      node = pdfium::MakeUnique<CXFA_Trace>(doc, packet);
      break;
    case XFA_Element::Float:
      node = pdfium::MakeUnique<CXFA_Float>(doc, packet);
      break;
    case XFA_Element::RenderPolicy:
      node = pdfium::MakeUnique<CXFA_RenderPolicy>(doc, packet);
      break;
    case XFA_Element::Destination:
      node = pdfium::MakeUnique<CXFA_Destination>(doc, packet);
      break;
    case XFA_Element::Value:
      node = pdfium::MakeUnique<CXFA_Value>(doc, packet);
      break;
    case XFA_Element::Bookend:
      node = pdfium::MakeUnique<CXFA_Bookend>(doc, packet);
      break;
    case XFA_Element::ExObject:
      node = pdfium::MakeUnique<CXFA_ExObject>(doc, packet);
      break;
    case XFA_Element::OpenAction:
      node = pdfium::MakeUnique<CXFA_OpenAction>(doc, packet);
      break;
    case XFA_Element::NeverEmbed:
      node = pdfium::MakeUnique<CXFA_NeverEmbed>(doc, packet);
      break;
    case XFA_Element::BindItems:
      node = pdfium::MakeUnique<CXFA_BindItems>(doc, packet);
      break;
    case XFA_Element::Calculate:
      node = pdfium::MakeUnique<CXFA_Calculate>(doc, packet);
      break;
    case XFA_Element::Print:
      node = pdfium::MakeUnique<CXFA_Print>(doc, packet);
      break;
    case XFA_Element::Extras:
      node = pdfium::MakeUnique<CXFA_Extras>(doc, packet);
      break;
    case XFA_Element::Proto:
      node = pdfium::MakeUnique<CXFA_Proto>(doc, packet);
      break;
    case XFA_Element::DSigData:
      node = pdfium::MakeUnique<CXFA_DSigData>(doc, packet);
      break;
    case XFA_Element::Creator:
      node = pdfium::MakeUnique<CXFA_Creator>(doc, packet);
      break;
    case XFA_Element::Connect:
      node = pdfium::MakeUnique<CXFA_Connect>(doc, packet);
      break;
    case XFA_Element::Permissions:
      node = pdfium::MakeUnique<CXFA_Permissions>(doc, packet);
      break;
    case XFA_Element::ConnectionSet:
      node = pdfium::MakeUnique<CXFA_ConnectionSet>(doc, packet);
      break;
    case XFA_Element::Submit:
      node = pdfium::MakeUnique<CXFA_Submit>(doc, packet);
      break;
    case XFA_Element::Range:
      node = pdfium::MakeUnique<CXFA_Range>(doc, packet);
      break;
    case XFA_Element::Linearized:
      node = pdfium::MakeUnique<CXFA_Linearized>(doc, packet);
      break;
    case XFA_Element::Packet:
      node = pdfium::MakeUnique<CXFA_Packet>(doc, packet);
      break;
    case XFA_Element::RootElement:
      node = pdfium::MakeUnique<CXFA_RootElement>(doc, packet);
      break;
    case XFA_Element::PlaintextMetadata:
      node = pdfium::MakeUnique<CXFA_PlaintextMetadata>(doc, packet);
      break;
    case XFA_Element::NumberSymbols:
      node = pdfium::MakeUnique<CXFA_NumberSymbols>(doc, packet);
      break;
    case XFA_Element::PrintHighQuality:
      node = pdfium::MakeUnique<CXFA_PrintHighQuality>(doc, packet);
      break;
    case XFA_Element::Driver:
      node = pdfium::MakeUnique<CXFA_Driver>(doc, packet);
      break;
    case XFA_Element::IncrementalLoad:
      node = pdfium::MakeUnique<CXFA_IncrementalLoad>(doc, packet);
      break;
    case XFA_Element::SubjectDN:
      node = pdfium::MakeUnique<CXFA_SubjectDN>(doc, packet);
      break;
    case XFA_Element::CompressLogicalStructure:
      node = pdfium::MakeUnique<CXFA_CompressLogicalStructure>(doc, packet);
      break;
    case XFA_Element::IncrementalMerge:
      node = pdfium::MakeUnique<CXFA_IncrementalMerge>(doc, packet);
      break;
    case XFA_Element::Radial:
      node = pdfium::MakeUnique<CXFA_Radial>(doc, packet);
      break;
    case XFA_Element::Variables:
      node = pdfium::MakeUnique<CXFA_Variables>(doc, packet);
      break;
    case XFA_Element::TimePatterns:
      node = pdfium::MakeUnique<CXFA_TimePatterns>(doc, packet);
      break;
    case XFA_Element::EffectiveInputPolicy:
      node = pdfium::MakeUnique<CXFA_EffectiveInputPolicy>(doc, packet);
      break;
    case XFA_Element::NameAttr:
      node = pdfium::MakeUnique<CXFA_NameAttr>(doc, packet);
      break;
    case XFA_Element::Conformance:
      node = pdfium::MakeUnique<CXFA_Conformance>(doc, packet);
      break;
    case XFA_Element::Transform:
      node = pdfium::MakeUnique<CXFA_Transform>(doc, packet);
      break;
    case XFA_Element::LockDocument:
      node = pdfium::MakeUnique<CXFA_LockDocument>(doc, packet);
      break;
    case XFA_Element::BreakAfter:
      node = pdfium::MakeUnique<CXFA_BreakAfter>(doc, packet);
      break;
    case XFA_Element::Line:
      node = pdfium::MakeUnique<CXFA_Line>(doc, packet);
      break;
    case XFA_Element::Source:
      node = pdfium::MakeUnique<CXFA_Source>(doc, packet);
      break;
    case XFA_Element::Occur:
      node = pdfium::MakeUnique<CXFA_Occur>(doc, packet);
      break;
    case XFA_Element::PickTrayByPDFSize:
      node = pdfium::MakeUnique<CXFA_PickTrayByPDFSize>(doc, packet);
      break;
    case XFA_Element::MonthNames:
      node = pdfium::MakeUnique<CXFA_MonthNames>(doc, packet);
      break;
    case XFA_Element::Severity:
      node = pdfium::MakeUnique<CXFA_Severity>(doc, packet);
      break;
    case XFA_Element::GroupParent:
      node = pdfium::MakeUnique<CXFA_GroupParent>(doc, packet);
      break;
    case XFA_Element::DocumentAssembly:
      node = pdfium::MakeUnique<CXFA_DocumentAssembly>(doc, packet);
      break;
    case XFA_Element::NumberSymbol:
      node = pdfium::MakeUnique<CXFA_NumberSymbol>(doc, packet);
      break;
    case XFA_Element::Tagged:
      node = pdfium::MakeUnique<CXFA_Tagged>(doc, packet);
      break;
    case XFA_Element::Items:
      node = pdfium::MakeUnique<CXFA_Items>(doc, packet);
      break;
    default:
      NOTREACHED();
      return nullptr;
  }
  if (!node || !node->IsValidInPacket(packet))
    return nullptr;
  return node;
}

// static
WideString CXFA_Node::AttributeToName(XFA_Attribute attr) {
  switch (attr) {
    case XFA_Attribute::H:
      return L"h";
    case XFA_Attribute::W:
      return L"w";
    case XFA_Attribute::X:
      return L"x";
    case XFA_Attribute::Y:
      return L"y";
    case XFA_Attribute::Id:
      return L"id";
    case XFA_Attribute::To:
      return L"to";
    case XFA_Attribute::LineThrough:
      return L"lineThrough";
    case XFA_Attribute::HAlign:
      return L"hAlign";
    case XFA_Attribute::Typeface:
      return L"typeface";
    case XFA_Attribute::BeforeTarget:
      return L"beforeTarget";
    case XFA_Attribute::Name:
      return L"name";
    case XFA_Attribute::Next:
      return L"next";
    case XFA_Attribute::DataRowCount:
      return L"dataRowCount";
    case XFA_Attribute::Break:
      return L"break";
    case XFA_Attribute::VScrollPolicy:
      return L"vScrollPolicy";
    case XFA_Attribute::FontHorizontalScale:
      return L"fontHorizontalScale";
    case XFA_Attribute::TextIndent:
      return L"textIndent";
    case XFA_Attribute::Context:
      return L"context";
    case XFA_Attribute::TrayOut:
      return L"trayOut";
    case XFA_Attribute::Cap:
      return L"cap";
    case XFA_Attribute::Max:
      return L"max";
    case XFA_Attribute::Min:
      return L"min";
    case XFA_Attribute::Ref:
      return L"ref";
    case XFA_Attribute::Rid:
      return L"rid";
    case XFA_Attribute::Url:
      return L"url";
    case XFA_Attribute::Use:
      return L"use";
    case XFA_Attribute::LeftInset:
      return L"leftInset";
    case XFA_Attribute::Widows:
      return L"widows";
    case XFA_Attribute::Level:
      return L"level";
    case XFA_Attribute::BottomInset:
      return L"bottomInset";
    case XFA_Attribute::OverflowTarget:
      return L"overflowTarget";
    case XFA_Attribute::AllowMacro:
      return L"allowMacro";
    case XFA_Attribute::PagePosition:
      return L"pagePosition";
    case XFA_Attribute::ColumnWidths:
      return L"columnWidths";
    case XFA_Attribute::OverflowLeader:
      return L"overflowLeader";
    case XFA_Attribute::Action:
      return L"action";
    case XFA_Attribute::NonRepudiation:
      return L"nonRepudiation";
    case XFA_Attribute::Rate:
      return L"rate";
    case XFA_Attribute::AllowRichText:
      return L"allowRichText";
    case XFA_Attribute::Role:
      return L"role";
    case XFA_Attribute::OverflowTrailer:
      return L"overflowTrailer";
    case XFA_Attribute::Operation:
      return L"operation";
    case XFA_Attribute::Timeout:
      return L"timeout";
    case XFA_Attribute::TopInset:
      return L"topInset";
    case XFA_Attribute::Access:
      return L"access";
    case XFA_Attribute::CommandType:
      return L"commandType";
    case XFA_Attribute::Format:
      return L"format";
    case XFA_Attribute::DataPrep:
      return L"dataPrep";
    case XFA_Attribute::WidgetData:
      return L"widgetData";
    case XFA_Attribute::Abbr:
      return L"abbr";
    case XFA_Attribute::MarginRight:
      return L"marginRight";
    case XFA_Attribute::DataDescription:
      return L"dataDescription";
    case XFA_Attribute::EncipherOnly:
      return L"encipherOnly";
    case XFA_Attribute::KerningMode:
      return L"kerningMode";
    case XFA_Attribute::Rotate:
      return L"rotate";
    case XFA_Attribute::WordCharacterCount:
      return L"wordCharacterCount";
    case XFA_Attribute::Type:
      return L"type";
    case XFA_Attribute::Reserve:
      return L"reserve";
    case XFA_Attribute::TextLocation:
      return L"textLocation";
    case XFA_Attribute::Priority:
      return L"priority";
    case XFA_Attribute::Underline:
      return L"underline";
    case XFA_Attribute::ModuleWidth:
      return L"moduleWidth";
    case XFA_Attribute::Hyphenate:
      return L"hyphenate";
    case XFA_Attribute::Listen:
      return L"listen";
    case XFA_Attribute::Delimiter:
      return L"delimiter";
    case XFA_Attribute::ContentType:
      return L"contentType";
    case XFA_Attribute::StartNew:
      return L"startNew";
    case XFA_Attribute::EofAction:
      return L"eofAction";
    case XFA_Attribute::AllowNeutral:
      return L"allowNeutral";
    case XFA_Attribute::Connection:
      return L"connection";
    case XFA_Attribute::BaselineShift:
      return L"baselineShift";
    case XFA_Attribute::OverlinePeriod:
      return L"overlinePeriod";
    case XFA_Attribute::FracDigits:
      return L"fracDigits";
    case XFA_Attribute::Orientation:
      return L"orientation";
    case XFA_Attribute::TimeStamp:
      return L"timeStamp";
    case XFA_Attribute::PrintCheckDigit:
      return L"printCheckDigit";
    case XFA_Attribute::MarginLeft:
      return L"marginLeft";
    case XFA_Attribute::Stroke:
      return L"stroke";
    case XFA_Attribute::ModuleHeight:
      return L"moduleHeight";
    case XFA_Attribute::TransferEncoding:
      return L"transferEncoding";
    case XFA_Attribute::Usage:
      return L"usage";
    case XFA_Attribute::Presence:
      return L"presence";
    case XFA_Attribute::RadixOffset:
      return L"radixOffset";
    case XFA_Attribute::Preserve:
      return L"preserve";
    case XFA_Attribute::AliasNode:
      return L"aliasNode";
    case XFA_Attribute::MultiLine:
      return L"multiLine";
    case XFA_Attribute::Version:
      return L"version";
    case XFA_Attribute::StartChar:
      return L"startChar";
    case XFA_Attribute::ScriptTest:
      return L"scriptTest";
    case XFA_Attribute::StartAngle:
      return L"startAngle";
    case XFA_Attribute::CursorType:
      return L"cursorType";
    case XFA_Attribute::DigitalSignature:
      return L"digitalSignature";
    case XFA_Attribute::CodeType:
      return L"codeType";
    case XFA_Attribute::Output:
      return L"output";
    case XFA_Attribute::BookendTrailer:
      return L"bookendTrailer";
    case XFA_Attribute::ImagingBBox:
      return L"imagingBBox";
    case XFA_Attribute::ExcludeInitialCap:
      return L"excludeInitialCap";
    case XFA_Attribute::Force:
      return L"force";
    case XFA_Attribute::CrlSign:
      return L"crlSign";
    case XFA_Attribute::Previous:
      return L"previous";
    case XFA_Attribute::PushCharacterCount:
      return L"pushCharacterCount";
    case XFA_Attribute::NullTest:
      return L"nullTest";
    case XFA_Attribute::RunAt:
      return L"runAt";
    case XFA_Attribute::SpaceBelow:
      return L"spaceBelow";
    case XFA_Attribute::SweepAngle:
      return L"sweepAngle";
    case XFA_Attribute::NumberOfCells:
      return L"numberOfCells";
    case XFA_Attribute::LetterSpacing:
      return L"letterSpacing";
    case XFA_Attribute::LockType:
      return L"lockType";
    case XFA_Attribute::PasswordChar:
      return L"passwordChar";
    case XFA_Attribute::VAlign:
      return L"vAlign";
    case XFA_Attribute::SourceBelow:
      return L"sourceBelow";
    case XFA_Attribute::Inverted:
      return L"inverted";
    case XFA_Attribute::Mark:
      return L"mark";
    case XFA_Attribute::MaxH:
      return L"maxH";
    case XFA_Attribute::MaxW:
      return L"maxW";
    case XFA_Attribute::Truncate:
      return L"truncate";
    case XFA_Attribute::MinH:
      return L"minH";
    case XFA_Attribute::MinW:
      return L"minW";
    case XFA_Attribute::Initial:
      return L"initial";
    case XFA_Attribute::Mode:
      return L"mode";
    case XFA_Attribute::Layout:
      return L"layout";
    case XFA_Attribute::Server:
      return L"server";
    case XFA_Attribute::EmbedPDF:
      return L"embedPDF";
    case XFA_Attribute::OddOrEven:
      return L"oddOrEven";
    case XFA_Attribute::TabDefault:
      return L"tabDefault";
    case XFA_Attribute::Contains:
      return L"contains";
    case XFA_Attribute::RightInset:
      return L"rightInset";
    case XFA_Attribute::MaxChars:
      return L"maxChars";
    case XFA_Attribute::Open:
      return L"open";
    case XFA_Attribute::Relation:
      return L"relation";
    case XFA_Attribute::WideNarrowRatio:
      return L"wideNarrowRatio";
    case XFA_Attribute::Relevant:
      return L"relevant";
    case XFA_Attribute::SignatureType:
      return L"signatureType";
    case XFA_Attribute::LineThroughPeriod:
      return L"lineThroughPeriod";
    case XFA_Attribute::Shape:
      return L"shape";
    case XFA_Attribute::TabStops:
      return L"tabStops";
    case XFA_Attribute::OutputBelow:
      return L"outputBelow";
    case XFA_Attribute::Short:
      return L"short";
    case XFA_Attribute::FontVerticalScale:
      return L"fontVerticalScale";
    case XFA_Attribute::Thickness:
      return L"thickness";
    case XFA_Attribute::CommitOn:
      return L"commitOn";
    case XFA_Attribute::RemainCharacterCount:
      return L"remainCharacterCount";
    case XFA_Attribute::KeyAgreement:
      return L"keyAgreement";
    case XFA_Attribute::ErrorCorrectionLevel:
      return L"errorCorrectionLevel";
    case XFA_Attribute::UpsMode:
      return L"upsMode";
    case XFA_Attribute::MergeMode:
      return L"mergeMode";
    case XFA_Attribute::Circular:
      return L"circular";
    case XFA_Attribute::PsName:
      return L"psName";
    case XFA_Attribute::Trailer:
      return L"trailer";
    case XFA_Attribute::UnicodeRange:
      return L"unicodeRange";
    case XFA_Attribute::ExecuteType:
      return L"executeType";
    case XFA_Attribute::DuplexImposition:
      return L"duplexImposition";
    case XFA_Attribute::TrayIn:
      return L"trayIn";
    case XFA_Attribute::BindingNode:
      return L"bindingNode";
    case XFA_Attribute::BofAction:
      return L"bofAction";
    case XFA_Attribute::Save:
      return L"save";
    case XFA_Attribute::TargetType:
      return L"targetType";
    case XFA_Attribute::KeyEncipherment:
      return L"keyEncipherment";
    case XFA_Attribute::CredentialServerPolicy:
      return L"credentialServerPolicy";
    case XFA_Attribute::Size:
      return L"size";
    case XFA_Attribute::InitialNumber:
      return L"initialNumber";
    case XFA_Attribute::Slope:
      return L"slope";
    case XFA_Attribute::CSpace:
      return L"cSpace";
    case XFA_Attribute::ColSpan:
      return L"colSpan";
    case XFA_Attribute::Binding:
      return L"binding";
    case XFA_Attribute::Checksum:
      return L"checksum";
    case XFA_Attribute::CharEncoding:
      return L"charEncoding";
    case XFA_Attribute::Bind:
      return L"bind";
    case XFA_Attribute::TextEntry:
      return L"textEntry";
    case XFA_Attribute::Archive:
      return L"archive";
    case XFA_Attribute::Uuid:
      return L"uuid";
    case XFA_Attribute::Posture:
      return L"posture";
    case XFA_Attribute::After:
      return L"after";
    case XFA_Attribute::Orphans:
      return L"orphans";
    case XFA_Attribute::QualifiedName:
      return L"qualifiedName";
    case XFA_Attribute::Usehref:
      return L"usehref";
    case XFA_Attribute::Locale:
      return L"locale";
    case XFA_Attribute::Weight:
      return L"weight";
    case XFA_Attribute::UnderlinePeriod:
      return L"underlinePeriod";
    case XFA_Attribute::Data:
      return L"data";
    case XFA_Attribute::Desc:
      return L"desc";
    case XFA_Attribute::Numbered:
      return L"numbered";
    case XFA_Attribute::DataColumnCount:
      return L"dataColumnCount";
    case XFA_Attribute::Overline:
      return L"overline";
    case XFA_Attribute::UrlPolicy:
      return L"urlPolicy";
    case XFA_Attribute::AnchorType:
      return L"anchorType";
    case XFA_Attribute::LabelRef:
      return L"labelRef";
    case XFA_Attribute::BookendLeader:
      return L"bookendLeader";
    case XFA_Attribute::MaxLength:
      return L"maxLength";
    case XFA_Attribute::AccessKey:
      return L"accessKey";
    case XFA_Attribute::CursorLocation:
      return L"cursorLocation";
    case XFA_Attribute::DelayedOpen:
      return L"delayedOpen";
    case XFA_Attribute::Target:
      return L"target";
    case XFA_Attribute::DataEncipherment:
      return L"dataEncipherment";
    case XFA_Attribute::AfterTarget:
      return L"afterTarget";
    case XFA_Attribute::Leader:
      return L"leader";
    case XFA_Attribute::Picker:
      return L"picker";
    case XFA_Attribute::From:
      return L"from";
    case XFA_Attribute::BaseProfile:
      return L"baseProfile";
    case XFA_Attribute::Aspect:
      return L"aspect";
    case XFA_Attribute::RowColumnRatio:
      return L"rowColumnRatio";
    case XFA_Attribute::LineHeight:
      return L"lineHeight";
    case XFA_Attribute::Highlight:
      return L"highlight";
    case XFA_Attribute::ValueRef:
      return L"valueRef";
    case XFA_Attribute::MaxEntries:
      return L"maxEntries";
    case XFA_Attribute::DataLength:
      return L"dataLength";
    case XFA_Attribute::Activity:
      return L"activity";
    case XFA_Attribute::Input:
      return L"input";
    case XFA_Attribute::Value:
      return L"value";
    case XFA_Attribute::BlankOrNotBlank:
      return L"blankOrNotBlank";
    case XFA_Attribute::AddRevocationInfo:
      return L"addRevocationInfo";
    case XFA_Attribute::GenericFamily:
      return L"genericFamily";
    case XFA_Attribute::Hand:
      return L"hand";
    case XFA_Attribute::Href:
      return L"href";
    case XFA_Attribute::TextEncoding:
      return L"textEncoding";
    case XFA_Attribute::LeadDigits:
      return L"leadDigits";
    case XFA_Attribute::Permissions:
      return L"permissions";
    case XFA_Attribute::SpaceAbove:
      return L"spaceAbove";
    case XFA_Attribute::CodeBase:
      return L"codeBase";
    case XFA_Attribute::Stock:
      return L"stock";
    case XFA_Attribute::IsNull:
      return L"isNull";
    case XFA_Attribute::RestoreState:
      return L"restoreState";
    case XFA_Attribute::ExcludeAllCaps:
      return L"excludeAllCaps";
    case XFA_Attribute::FormatTest:
      return L"formatTest";
    case XFA_Attribute::HScrollPolicy:
      return L"hScrollPolicy";
    case XFA_Attribute::Join:
      return L"join";
    case XFA_Attribute::KeyCertSign:
      return L"keyCertSign";
    case XFA_Attribute::Radius:
      return L"radius";
    case XFA_Attribute::SourceAbove:
      return L"sourceAbove";
    case XFA_Attribute::Override:
      return L"override";
    case XFA_Attribute::ClassId:
      return L"classId";
    case XFA_Attribute::Disable:
      return L"disable";
    case XFA_Attribute::Scope:
      return L"scope";
    case XFA_Attribute::Match:
      return L"match";
    case XFA_Attribute::Placement:
      return L"placement";
    case XFA_Attribute::Before:
      return L"before";
    case XFA_Attribute::WritingScript:
      return L"writingScript";
    case XFA_Attribute::EndChar:
      return L"endChar";
    case XFA_Attribute::Lock:
      return L"lock";
    case XFA_Attribute::Long:
      return L"long";
    case XFA_Attribute::Intact:
      return L"intact";
    case XFA_Attribute::XdpContent:
      return L"xdpContent";
    case XFA_Attribute::DecipherOnly:
      return L"decipherOnly";

    default:
      NOTREACHED();
      break;
  }
  return L"";
}

#ifndef NDEBUG
// static
WideString CXFA_Node::ElementToName(XFA_Element attr) {
  switch (attr) {
    case XFA_Element::Ps:
      return L"ps";
    case XFA_Element::To:
      return L"to";
    case XFA_Element::Ui:
      return L"ui";
    case XFA_Element::RecordSet:
      return L"recordSet";
    case XFA_Element::SubsetBelow:
      return L"subsetBelow";
    case XFA_Element::SubformSet:
      return L"subformSet";
    case XFA_Element::AdobeExtensionLevel:
      return L"adobeExtensionLevel";
    case XFA_Element::Typeface:
      return L"typeface";
    case XFA_Element::Break:
      return L"break";
    case XFA_Element::FontInfo:
      return L"fontInfo";
    case XFA_Element::NumberPattern:
      return L"numberPattern";
    case XFA_Element::DynamicRender:
      return L"dynamicRender";
    case XFA_Element::PrintScaling:
      return L"printScaling";
    case XFA_Element::CheckButton:
      return L"checkButton";
    case XFA_Element::DatePatterns:
      return L"datePatterns";
    case XFA_Element::SourceSet:
      return L"sourceSet";
    case XFA_Element::Amd:
      return L"amd";
    case XFA_Element::Arc:
      return L"arc";
    case XFA_Element::Day:
      return L"day";
    case XFA_Element::Era:
      return L"era";
    case XFA_Element::Jog:
      return L"jog";
    case XFA_Element::Log:
      return L"log";
    case XFA_Element::Map:
      return L"map";
    case XFA_Element::Mdp:
      return L"mdp";
    case XFA_Element::BreakBefore:
      return L"breakBefore";
    case XFA_Element::Oid:
      return L"oid";
    case XFA_Element::Pcl:
      return L"pcl";
    case XFA_Element::Pdf:
      return L"pdf";
    case XFA_Element::Ref:
      return L"ref";
    case XFA_Element::Uri:
      return L"uri";
    case XFA_Element::Xdc:
      return L"xdc";
    case XFA_Element::Xdp:
      return L"xdp";
    case XFA_Element::Xfa:
      return L"xfa";
    case XFA_Element::Xsl:
      return L"xsl";
    case XFA_Element::Zpl:
      return L"zpl";
    case XFA_Element::Cache:
      return L"cache";
    case XFA_Element::Margin:
      return L"margin";
    case XFA_Element::KeyUsage:
      return L"keyUsage";
    case XFA_Element::Exclude:
      return L"exclude";
    case XFA_Element::ChoiceList:
      return L"choiceList";
    case XFA_Element::Level:
      return L"level";
    case XFA_Element::LabelPrinter:
      return L"labelPrinter";
    case XFA_Element::CalendarSymbols:
      return L"calendarSymbols";
    case XFA_Element::Para:
      return L"para";
    case XFA_Element::Part:
      return L"part";
    case XFA_Element::Pdfa:
      return L"pdfa";
    case XFA_Element::Filter:
      return L"filter";
    case XFA_Element::Present:
      return L"present";
    case XFA_Element::Pagination:
      return L"pagination";
    case XFA_Element::Encoding:
      return L"encoding";
    case XFA_Element::Event:
      return L"event";
    case XFA_Element::Whitespace:
      return L"whitespace";
    case XFA_Element::DefaultUi:
      return L"defaultUi";
    case XFA_Element::DataModel:
      return L"dataModel";
    case XFA_Element::Barcode:
      return L"barcode";
    case XFA_Element::TimePattern:
      return L"timePattern";
    case XFA_Element::BatchOutput:
      return L"batchOutput";
    case XFA_Element::Enforce:
      return L"enforce";
    case XFA_Element::CurrencySymbols:
      return L"currencySymbols";
    case XFA_Element::AddSilentPrint:
      return L"addSilentPrint";
    case XFA_Element::Rename:
      return L"rename";
    case XFA_Element::Operation:
      return L"operation";
    case XFA_Element::Typefaces:
      return L"typefaces";
    case XFA_Element::SubjectDNs:
      return L"subjectDNs";
    case XFA_Element::Issuers:
      return L"issuers";
    case XFA_Element::WsdlConnection:
      return L"wsdlConnection";
    case XFA_Element::Debug:
      return L"debug";
    case XFA_Element::Delta:
      return L"delta";
    case XFA_Element::EraNames:
      return L"eraNames";
    case XFA_Element::ModifyAnnots:
      return L"modifyAnnots";
    case XFA_Element::StartNode:
      return L"startNode";
    case XFA_Element::Button:
      return L"button";
    case XFA_Element::Format:
      return L"format";
    case XFA_Element::Border:
      return L"border";
    case XFA_Element::Area:
      return L"area";
    case XFA_Element::Hyphenation:
      return L"hyphenation";
    case XFA_Element::Text:
      return L"text";
    case XFA_Element::Time:
      return L"time";
    case XFA_Element::Type:
      return L"type";
    case XFA_Element::Overprint:
      return L"overprint";
    case XFA_Element::Certificates:
      return L"certificates";
    case XFA_Element::EncryptionMethods:
      return L"encryptionMethods";
    case XFA_Element::SetProperty:
      return L"setProperty";
    case XFA_Element::PrinterName:
      return L"printerName";
    case XFA_Element::StartPage:
      return L"startPage";
    case XFA_Element::PageOffset:
      return L"pageOffset";
    case XFA_Element::DateTime:
      return L"dateTime";
    case XFA_Element::Comb:
      return L"comb";
    case XFA_Element::Pattern:
      return L"pattern";
    case XFA_Element::IfEmpty:
      return L"ifEmpty";
    case XFA_Element::SuppressBanner:
      return L"suppressBanner";
    case XFA_Element::OutputBin:
      return L"outputBin";
    case XFA_Element::Field:
      return L"field";
    case XFA_Element::Agent:
      return L"agent";
    case XFA_Element::OutputXSL:
      return L"outputXSL";
    case XFA_Element::AdjustData:
      return L"adjustData";
    case XFA_Element::AutoSave:
      return L"autoSave";
    case XFA_Element::ContentArea:
      return L"contentArea";
    case XFA_Element::WsdlAddress:
      return L"wsdlAddress";
    case XFA_Element::Solid:
      return L"solid";
    case XFA_Element::DateTimeSymbols:
      return L"dateTimeSymbols";
    case XFA_Element::EncryptionLevel:
      return L"encryptionLevel";
    case XFA_Element::Edge:
      return L"edge";
    case XFA_Element::Stipple:
      return L"stipple";
    case XFA_Element::Attributes:
      return L"attributes";
    case XFA_Element::VersionControl:
      return L"versionControl";
    case XFA_Element::Meridiem:
      return L"meridiem";
    case XFA_Element::ExclGroup:
      return L"exclGroup";
    case XFA_Element::ToolTip:
      return L"toolTip";
    case XFA_Element::Compress:
      return L"compress";
    case XFA_Element::Reason:
      return L"reason";
    case XFA_Element::Execute:
      return L"execute";
    case XFA_Element::ContentCopy:
      return L"contentCopy";
    case XFA_Element::DateTimeEdit:
      return L"dateTimeEdit";
    case XFA_Element::Config:
      return L"config";
    case XFA_Element::Image:
      return L"image";
    case XFA_Element::SharpxHTML:
      return L"#xHTML";
    case XFA_Element::NumberOfCopies:
      return L"numberOfCopies";
    case XFA_Element::BehaviorOverride:
      return L"behaviorOverride";
    case XFA_Element::TimeStamp:
      return L"timeStamp";
    case XFA_Element::Month:
      return L"month";
    case XFA_Element::ViewerPreferences:
      return L"viewerPreferences";
    case XFA_Element::ScriptModel:
      return L"scriptModel";
    case XFA_Element::Decimal:
      return L"decimal";
    case XFA_Element::Subform:
      return L"subform";
    case XFA_Element::Select:
      return L"select";
    case XFA_Element::Window:
      return L"window";
    case XFA_Element::LocaleSet:
      return L"localeSet";
    case XFA_Element::Handler:
      return L"handler";
    case XFA_Element::Presence:
      return L"presence";
    case XFA_Element::Record:
      return L"record";
    case XFA_Element::Embed:
      return L"embed";
    case XFA_Element::Version:
      return L"version";
    case XFA_Element::Command:
      return L"command";
    case XFA_Element::Copies:
      return L"copies";
    case XFA_Element::Staple:
      return L"staple";
    case XFA_Element::SubmitFormat:
      return L"submitFormat";
    case XFA_Element::Boolean:
      return L"boolean";
    case XFA_Element::Message:
      return L"message";
    case XFA_Element::Output:
      return L"output";
    case XFA_Element::PsMap:
      return L"psMap";
    case XFA_Element::ExcludeNS:
      return L"excludeNS";
    case XFA_Element::Assist:
      return L"assist";
    case XFA_Element::Picture:
      return L"picture";
    case XFA_Element::Traversal:
      return L"traversal";
    case XFA_Element::SilentPrint:
      return L"silentPrint";
    case XFA_Element::WebClient:
      return L"webClient";
    case XFA_Element::Producer:
      return L"producer";
    case XFA_Element::Corner:
      return L"corner";
    case XFA_Element::MsgId:
      return L"msgId";
    case XFA_Element::Color:
      return L"color";
    case XFA_Element::Keep:
      return L"keep";
    case XFA_Element::Query:
      return L"query";
    case XFA_Element::Insert:
      return L"insert";
    case XFA_Element::ImageEdit:
      return L"imageEdit";
    case XFA_Element::Validate:
      return L"validate";
    case XFA_Element::DigestMethods:
      return L"digestMethods";
    case XFA_Element::NumberPatterns:
      return L"numberPatterns";
    case XFA_Element::PageSet:
      return L"pageSet";
    case XFA_Element::Integer:
      return L"integer";
    case XFA_Element::SoapAddress:
      return L"soapAddress";
    case XFA_Element::Equate:
      return L"equate";
    case XFA_Element::FormFieldFilling:
      return L"formFieldFilling";
    case XFA_Element::PageRange:
      return L"pageRange";
    case XFA_Element::Update:
      return L"update";
    case XFA_Element::ConnectString:
      return L"connectString";
    case XFA_Element::Mode:
      return L"mode";
    case XFA_Element::Layout:
      return L"layout";
    case XFA_Element::Sharpxml:
      return L"#xml";
    case XFA_Element::XsdConnection:
      return L"xsdConnection";
    case XFA_Element::Traverse:
      return L"traverse";
    case XFA_Element::Encodings:
      return L"encodings";
    case XFA_Element::Template:
      return L"template";
    case XFA_Element::Acrobat:
      return L"acrobat";
    case XFA_Element::ValidationMessaging:
      return L"validationMessaging";
    case XFA_Element::Signing:
      return L"signing";
    case XFA_Element::Script:
      return L"script";
    case XFA_Element::AddViewerPreferences:
      return L"addViewerPreferences";
    case XFA_Element::AlwaysEmbed:
      return L"alwaysEmbed";
    case XFA_Element::PasswordEdit:
      return L"passwordEdit";
    case XFA_Element::NumericEdit:
      return L"numericEdit";
    case XFA_Element::EncryptionMethod:
      return L"encryptionMethod";
    case XFA_Element::Change:
      return L"change";
    case XFA_Element::PageArea:
      return L"pageArea";
    case XFA_Element::SubmitUrl:
      return L"submitUrl";
    case XFA_Element::Oids:
      return L"oids";
    case XFA_Element::Signature:
      return L"signature";
    case XFA_Element::ADBE_JSConsole:
      return L"ADBE_JSConsole";
    case XFA_Element::Caption:
      return L"caption";
    case XFA_Element::Relevant:
      return L"relevant";
    case XFA_Element::FlipLabel:
      return L"flipLabel";
    case XFA_Element::ExData:
      return L"exData";
    case XFA_Element::DayNames:
      return L"dayNames";
    case XFA_Element::SoapAction:
      return L"soapAction";
    case XFA_Element::DefaultTypeface:
      return L"defaultTypeface";
    case XFA_Element::Manifest:
      return L"manifest";
    case XFA_Element::Overflow:
      return L"overflow";
    case XFA_Element::Linear:
      return L"linear";
    case XFA_Element::CurrencySymbol:
      return L"currencySymbol";
    case XFA_Element::Delete:
      return L"delete";
    case XFA_Element::Deltas:
      return L"deltas";
    case XFA_Element::DigestMethod:
      return L"digestMethod";
    case XFA_Element::InstanceManager:
      return L"instanceManager";
    case XFA_Element::EquateRange:
      return L"equateRange";
    case XFA_Element::Medium:
      return L"medium";
    case XFA_Element::TextEdit:
      return L"textEdit";
    case XFA_Element::TemplateCache:
      return L"templateCache";
    case XFA_Element::CompressObjectStream:
      return L"compressObjectStream";
    case XFA_Element::DataValue:
      return L"dataValue";
    case XFA_Element::AccessibleContent:
      return L"accessibleContent";
    case XFA_Element::IncludeXDPContent:
      return L"includeXDPContent";
    case XFA_Element::XmlConnection:
      return L"xmlConnection";
    case XFA_Element::ValidateApprovalSignatures:
      return L"validateApprovalSignatures";
    case XFA_Element::SignData:
      return L"signData";
    case XFA_Element::Packets:
      return L"packets";
    case XFA_Element::DatePattern:
      return L"datePattern";
    case XFA_Element::DuplexOption:
      return L"duplexOption";
    case XFA_Element::Base:
      return L"base";
    case XFA_Element::Bind:
      return L"bind";
    case XFA_Element::Compression:
      return L"compression";
    case XFA_Element::User:
      return L"user";
    case XFA_Element::Rectangle:
      return L"rectangle";
    case XFA_Element::EffectiveOutputPolicy:
      return L"effectiveOutputPolicy";
    case XFA_Element::ADBE_JSDebugger:
      return L"ADBE_JSDebugger";
    case XFA_Element::Acrobat7:
      return L"acrobat7";
    case XFA_Element::Interactive:
      return L"interactive";
    case XFA_Element::Locale:
      return L"locale";
    case XFA_Element::CurrentPage:
      return L"currentPage";
    case XFA_Element::Data:
      return L"data";
    case XFA_Element::Date:
      return L"date";
    case XFA_Element::Desc:
      return L"desc";
    case XFA_Element::Encrypt:
      return L"encrypt";
    case XFA_Element::Draw:
      return L"draw";
    case XFA_Element::Encryption:
      return L"encryption";
    case XFA_Element::MeridiemNames:
      return L"meridiemNames";
    case XFA_Element::Messaging:
      return L"messaging";
    case XFA_Element::Speak:
      return L"speak";
    case XFA_Element::DataGroup:
      return L"dataGroup";
    case XFA_Element::Common:
      return L"common";
    case XFA_Element::Sharptext:
      return L"#text";
    case XFA_Element::PaginationOverride:
      return L"paginationOverride";
    case XFA_Element::Reasons:
      return L"reasons";
    case XFA_Element::SignatureProperties:
      return L"signatureProperties";
    case XFA_Element::Threshold:
      return L"threshold";
    case XFA_Element::AppearanceFilter:
      return L"appearanceFilter";
    case XFA_Element::Fill:
      return L"fill";
    case XFA_Element::Font:
      return L"font";
    case XFA_Element::Form:
      return L"form";
    case XFA_Element::MediumInfo:
      return L"mediumInfo";
    case XFA_Element::Certificate:
      return L"certificate";
    case XFA_Element::Password:
      return L"password";
    case XFA_Element::RunScripts:
      return L"runScripts";
    case XFA_Element::Trace:
      return L"trace";
    case XFA_Element::Float:
      return L"float";
    case XFA_Element::RenderPolicy:
      return L"renderPolicy";
    case XFA_Element::Destination:
      return L"destination";
    case XFA_Element::Value:
      return L"value";
    case XFA_Element::Bookend:
      return L"bookend";
    case XFA_Element::ExObject:
      return L"exObject";
    case XFA_Element::OpenAction:
      return L"openAction";
    case XFA_Element::NeverEmbed:
      return L"neverEmbed";
    case XFA_Element::BindItems:
      return L"bindItems";
    case XFA_Element::Calculate:
      return L"calculate";
    case XFA_Element::Print:
      return L"print";
    case XFA_Element::Extras:
      return L"extras";
    case XFA_Element::Proto:
      return L"proto";
    case XFA_Element::DSigData:
      return L"dSigData";
    case XFA_Element::Creator:
      return L"creator";
    case XFA_Element::Connect:
      return L"connect";
    case XFA_Element::Permissions:
      return L"permissions";
    case XFA_Element::ConnectionSet:
      return L"connectionSet";
    case XFA_Element::Submit:
      return L"submit";
    case XFA_Element::Range:
      return L"range";
    case XFA_Element::Linearized:
      return L"linearized";
    case XFA_Element::Packet:
      return L"packet";
    case XFA_Element::RootElement:
      return L"rootElement";
    case XFA_Element::PlaintextMetadata:
      return L"plaintextMetadata";
    case XFA_Element::NumberSymbols:
      return L"numberSymbols";
    case XFA_Element::PrintHighQuality:
      return L"printHighQuality";
    case XFA_Element::Driver:
      return L"driver";
    case XFA_Element::IncrementalLoad:
      return L"incrementalLoad";
    case XFA_Element::SubjectDN:
      return L"subjectDN";
    case XFA_Element::CompressLogicalStructure:
      return L"compressLogicalStructure";
    case XFA_Element::IncrementalMerge:
      return L"incrementalMerge";
    case XFA_Element::Radial:
      return L"radial";
    case XFA_Element::Variables:
      return L"variables";
    case XFA_Element::TimePatterns:
      return L"timePatterns";
    case XFA_Element::EffectiveInputPolicy:
      return L"effectiveInputPolicy";
    case XFA_Element::NameAttr:
      return L"nameAttr";
    case XFA_Element::Conformance:
      return L"conformance";
    case XFA_Element::Transform:
      return L"transform";
    case XFA_Element::LockDocument:
      return L"lockDocument";
    case XFA_Element::BreakAfter:
      return L"breakAfter";
    case XFA_Element::Line:
      return L"line";
    case XFA_Element::Source:
      return L"source";
    case XFA_Element::Occur:
      return L"occur";
    case XFA_Element::PickTrayByPDFSize:
      return L"pickTrayByPDFSize";
    case XFA_Element::MonthNames:
      return L"monthNames";
    case XFA_Element::Severity:
      return L"severity";
    case XFA_Element::GroupParent:
      return L"groupParent";
    case XFA_Element::DocumentAssembly:
      return L"documentAssembly";
    case XFA_Element::NumberSymbol:
      return L"numberSymbol";
    case XFA_Element::Tagged:
      return L"tagged";
    case XFA_Element::Items:
      return L"items";

    default:
      NOTREACHED();
      break;
  }
  return L"";
}
#endif  // NDEBUG

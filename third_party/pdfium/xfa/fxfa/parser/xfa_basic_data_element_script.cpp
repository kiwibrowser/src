// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/parser/xfa_basic_data.h"

#include "fxjs/xfa/cjx_arc.h"
#include "fxjs/xfa/cjx_area.h"
#include "fxjs/xfa/cjx_assist.h"
#include "fxjs/xfa/cjx_barcode.h"
#include "fxjs/xfa/cjx_bind.h"
#include "fxjs/xfa/cjx_binditems.h"
#include "fxjs/xfa/cjx_bookend.h"
#include "fxjs/xfa/cjx_boolean.h"
#include "fxjs/xfa/cjx_border.h"
#include "fxjs/xfa/cjx_break.h"
#include "fxjs/xfa/cjx_breakafter.h"
#include "fxjs/xfa/cjx_breakbefore.h"
#include "fxjs/xfa/cjx_button.h"
#include "fxjs/xfa/cjx_calculate.h"
#include "fxjs/xfa/cjx_caption.h"
#include "fxjs/xfa/cjx_certificate.h"
#include "fxjs/xfa/cjx_certificates.h"
#include "fxjs/xfa/cjx_checkbutton.h"
#include "fxjs/xfa/cjx_choicelist.h"
#include "fxjs/xfa/cjx_color.h"
#include "fxjs/xfa/cjx_comb.h"
#include "fxjs/xfa/cjx_command.h"
#include "fxjs/xfa/cjx_connect.h"
#include "fxjs/xfa/cjx_connectstring.h"
#include "fxjs/xfa/cjx_contentarea.h"
#include "fxjs/xfa/cjx_corner.h"
#include "fxjs/xfa/cjx_datavalue.h"
#include "fxjs/xfa/cjx_datawindow.h"
#include "fxjs/xfa/cjx_date.h"
#include "fxjs/xfa/cjx_datetime.h"
#include "fxjs/xfa/cjx_datetimeedit.h"
#include "fxjs/xfa/cjx_decimal.h"
#include "fxjs/xfa/cjx_defaultui.h"
#include "fxjs/xfa/cjx_delete.h"
#include "fxjs/xfa/cjx_delta.h"
#include "fxjs/xfa/cjx_desc.h"
#include "fxjs/xfa/cjx_digestmethod.h"
#include "fxjs/xfa/cjx_digestmethods.h"
#include "fxjs/xfa/cjx_draw.h"
#include "fxjs/xfa/cjx_edge.h"
#include "fxjs/xfa/cjx_encoding.h"
#include "fxjs/xfa/cjx_encodings.h"
#include "fxjs/xfa/cjx_encrypt.h"
#include "fxjs/xfa/cjx_event.h"
#include "fxjs/xfa/cjx_eventpseudomodel.h"
#include "fxjs/xfa/cjx_exclgroup.h"
#include "fxjs/xfa/cjx_exdata.h"
#include "fxjs/xfa/cjx_execute.h"
#include "fxjs/xfa/cjx_exobject.h"
#include "fxjs/xfa/cjx_extras.h"
#include "fxjs/xfa/cjx_field.h"
#include "fxjs/xfa/cjx_fill.h"
#include "fxjs/xfa/cjx_filter.h"
#include "fxjs/xfa/cjx_float.h"
#include "fxjs/xfa/cjx_font.h"
#include "fxjs/xfa/cjx_format.h"
#include "fxjs/xfa/cjx_handler.h"
#include "fxjs/xfa/cjx_hostpseudomodel.h"
#include "fxjs/xfa/cjx_image.h"
#include "fxjs/xfa/cjx_imageedit.h"
#include "fxjs/xfa/cjx_insert.h"
#include "fxjs/xfa/cjx_instancemanager.h"
#include "fxjs/xfa/cjx_integer.h"
#include "fxjs/xfa/cjx_issuers.h"
#include "fxjs/xfa/cjx_items.h"
#include "fxjs/xfa/cjx_keep.h"
#include "fxjs/xfa/cjx_keyusage.h"
#include "fxjs/xfa/cjx_layoutpseudomodel.h"
#include "fxjs/xfa/cjx_line.h"
#include "fxjs/xfa/cjx_linear.h"
#include "fxjs/xfa/cjx_logpseudomodel.h"
#include "fxjs/xfa/cjx_manifest.h"
#include "fxjs/xfa/cjx_map.h"
#include "fxjs/xfa/cjx_margin.h"
#include "fxjs/xfa/cjx_mdp.h"
#include "fxjs/xfa/cjx_medium.h"
#include "fxjs/xfa/cjx_message.h"
#include "fxjs/xfa/cjx_node.h"
#include "fxjs/xfa/cjx_numericedit.h"
#include "fxjs/xfa/cjx_occur.h"
#include "fxjs/xfa/cjx_oid.h"
#include "fxjs/xfa/cjx_oids.h"
#include "fxjs/xfa/cjx_operation.h"
#include "fxjs/xfa/cjx_overflow.h"
#include "fxjs/xfa/cjx_packet.h"
#include "fxjs/xfa/cjx_pagearea.h"
#include "fxjs/xfa/cjx_pageset.h"
#include "fxjs/xfa/cjx_para.h"
#include "fxjs/xfa/cjx_password.h"
#include "fxjs/xfa/cjx_passwordedit.h"
#include "fxjs/xfa/cjx_pattern.h"
#include "fxjs/xfa/cjx_picture.h"
#include "fxjs/xfa/cjx_query.h"
#include "fxjs/xfa/cjx_radial.h"
#include "fxjs/xfa/cjx_reason.h"
#include "fxjs/xfa/cjx_reasons.h"
#include "fxjs/xfa/cjx_recordset.h"
#include "fxjs/xfa/cjx_rectangle.h"
#include "fxjs/xfa/cjx_ref.h"
#include "fxjs/xfa/cjx_rootelement.h"
#include "fxjs/xfa/cjx_script.h"
#include "fxjs/xfa/cjx_select.h"
#include "fxjs/xfa/cjx_setproperty.h"
#include "fxjs/xfa/cjx_signature.h"
#include "fxjs/xfa/cjx_signatureproperties.h"
#include "fxjs/xfa/cjx_signaturepseudomodel.h"
#include "fxjs/xfa/cjx_signdata.h"
#include "fxjs/xfa/cjx_signing.h"
#include "fxjs/xfa/cjx_soapaction.h"
#include "fxjs/xfa/cjx_soapaddress.h"
#include "fxjs/xfa/cjx_solid.h"
#include "fxjs/xfa/cjx_source.h"
#include "fxjs/xfa/cjx_sourceset.h"
#include "fxjs/xfa/cjx_speak.h"
#include "fxjs/xfa/cjx_stipple.h"
#include "fxjs/xfa/cjx_subform.h"
#include "fxjs/xfa/cjx_subformset.h"
#include "fxjs/xfa/cjx_subjectdn.h"
#include "fxjs/xfa/cjx_subjectdns.h"
#include "fxjs/xfa/cjx_submit.h"
#include "fxjs/xfa/cjx_text.h"
#include "fxjs/xfa/cjx_textedit.h"
#include "fxjs/xfa/cjx_time.h"
#include "fxjs/xfa/cjx_timestamp.h"
#include "fxjs/xfa/cjx_tooltip.h"
#include "fxjs/xfa/cjx_traversal.h"
#include "fxjs/xfa/cjx_traverse.h"
#include "fxjs/xfa/cjx_tree.h"
#include "fxjs/xfa/cjx_treelist.h"
#include "fxjs/xfa/cjx_ui.h"
#include "fxjs/xfa/cjx_update.h"
#include "fxjs/xfa/cjx_uri.h"
#include "fxjs/xfa/cjx_user.h"
#include "fxjs/xfa/cjx_validate.h"
#include "fxjs/xfa/cjx_value.h"
#include "fxjs/xfa/cjx_variables.h"
#include "fxjs/xfa/cjx_wsdladdress.h"
#include "fxjs/xfa/cjx_wsdlconnection.h"
#include "fxjs/xfa/cjx_xfa.h"
#include "fxjs/xfa/cjx_xmlconnection.h"
#include "fxjs/xfa/cjx_xsdconnection.h"
#include "xfa/fxfa/fxfa_basic.h"

const XFA_SCRIPTHIERARCHY g_XFAScriptIndex[] = {
    {/* ps */ 0, 2, 316},
    {/* to */ 2, 2, 316},
    {/* ui */ 4, 2, 316},
    {/* recordSet */ 6, 8, 316},
    {/* subsetBelow */ 14, 4, 316},
    {/* subformSet */ 18, 5, 317},
    {/* adobeExtensionLevel */ 23, 2, 316},
    {/* typeface */ 25, 1, 316},
    {/* break */ 26, 12, 316},
    {/* fontInfo */ 38, 2, 316},
    {/* numberPattern */ 40, 1, 316},
    {/* dynamicRender */ 41, 3, 316},
    {/* printScaling */ 44, 2, 316},
    {/* checkButton */ 46, 6, 316},
    {/* datePatterns */ 52, 0, 316},
    {/* sourceSet */ 52, 2, 319},
    {/* amd */ 54, 2, 316},
    {/* arc */ 56, 6, 316},
    {/* day */ 62, 0, 316},
    {/* era */ 62, 0, 316},
    {/* jog */ 62, 2, 316},
    {/* log */ 64, 2, 316},
    {/* map */ 66, 6, 316},
    {/* mdp */ 72, 4, 316},
    {/* breakBefore */ 76, 7, 316},
    {/* oid */ 83, 2, 320},
    {/* pcl */ 85, 3, 316},
    {/* pdf */ 88, 3, 316},
    {/* ref */ 91, 2, 320},
    {/* uri */ 93, 6, 320},
    {/* xdc */ 99, 4, 316},
    {/* xdp */ 103, 2, 316},
    {/* xfa */ 105, 3, 319},
    {/* xsl */ 108, 4, 316},
    {/* zpl */ 112, 3, 316},
    {/* cache */ 115, 2, 316},
    {/* margin */ 117, 6, 316},
    {/* keyUsage */ 123, 12, 316},
    {/* exclude */ 135, 2, 316},
    {/* choiceList */ 137, 5, 316},
    {/* level */ 142, 2, 316},
    {/* labelPrinter */ 144, 3, 316},
    {/* calendarSymbols */ 147, 1, 316},
    {/* para */ 148, 14, 316},
    {/* part */ 162, 2, 316},
    {/* pdfa */ 164, 2, 316},
    {/* filter */ 166, 3, 316},
    {/* present */ 169, 2, 316},
    {/* pagination */ 171, 2, 316},
    {/* encoding */ 173, 2, 316},
    {/* event */ 175, 4, 316},
    {/* whitespace */ 179, 2, 316},
    {/* defaultUi */ 181, 2, 316},
    {/* dataModel */ 183, 0, 319},
    {/* barcode */ 183, 20, 316},
    {/* timePattern */ 203, 1, 316},
    {/* batchOutput */ 204, 3, 316},
    {/* enforce */ 207, 2, 316},
    {/* currencySymbols */ 209, 0, 316},
    {/* addSilentPrint */ 209, 2, 316},
    {/* rename */ 211, 2, 316},
    {/* operation */ 213, 4, 320},
    {/* typefaces */ 217, 0, 316},
    {/* subjectDNs */ 217, 1, 316},
    {/* issuers */ 218, 3, 316},
    {/* signaturePseudoModel */ 221, 0, 312},
    {/* wsdlConnection */ 221, 1, 316},
    {/* debug */ 222, 2, 316},
    {/* delta */ 224, 3, -1},
    {/* eraNames */ 227, 0, 316},
    {/* modifyAnnots */ 227, 2, 316},
    {/* startNode */ 229, 2, 316},
    {/* button */ 231, 3, 316},
    {/* format */ 234, 2, 316},
    {/* border */ 236, 6, 316},
    {/* area */ 242, 10, 317},
    {/* hyphenation */ 252, 9, 316},
    {/* text */ 261, 5, 318},
    {/* time */ 266, 4, 318},
    {/* type */ 270, 2, 316},
    {/* overprint */ 272, 2, 316},
    {/* certificates */ 274, 5, 316},
    {/* encryptionMethods */ 279, 3, 316},
    {/* setProperty */ 282, 2, 316},
    {/* printerName */ 284, 2, 316},
    {/* startPage */ 286, 2, 316},
    {/* pageOffset */ 288, 2, 316},
    {/* dateTime */ 290, 4, 316},
    {/* comb */ 294, 3, 316},
    {/* pattern */ 297, 3, 316},
    {/* ifEmpty */ 300, 2, 316},
    {/* suppressBanner */ 302, 2, 316},
    {/* outputBin */ 304, 2, 316},
    {/* field */ 306, 36, 317},
    {/* agent */ 342, 3, 316},
    {/* outputXSL */ 345, 2, 316},
    {/* adjustData */ 347, 2, 316},
    {/* autoSave */ 349, 2, 316},
    {/* contentArea */ 351, 7, 317},
    {/* eventPseudoModel */ 358, 16, 312},
    {/* wsdlAddress */ 374, 2, 320},
    {/* solid */ 376, 2, 316},
    {/* dateTimeSymbols */ 378, 0, 316},
    {/* encryptionLevel */ 378, 2, 316},
    {/* edge */ 380, 6, 316},
    {/* stipple */ 386, 3, 316},
    {/* attributes */ 389, 2, 316},
    {/* versionControl */ 391, 4, 316},
    {/* meridiem */ 395, 0, 316},
    {/* exclGroup */ 395, 30, 316},
    {/* toolTip */ 425, 2, 320},
    {/* compress */ 427, 3, 316},
    {/* reason */ 430, 2, 320},
    {/* execute */ 432, 5, 316},
    {/* contentCopy */ 437, 2, 316},
    {/* dateTimeEdit */ 439, 3, 316},
    {/* config */ 442, 2, 316},
    {/* image */ 444, 8, 316},
    {/* #xHTML */ 452, 1, 316},
    {/* numberOfCopies */ 453, 2, 316},
    {/* behaviorOverride */ 455, 2, 316},
    {/* timeStamp */ 457, 4, 316},
    {/* month */ 461, 0, 316},
    {/* viewerPreferences */ 461, 2, 316},
    {/* scriptModel */ 463, 2, 316},
    {/* decimal */ 465, 6, 318},
    {/* subform */ 471, 31, 317},
    {/* select */ 502, 2, 320},
    {/* window */ 504, 2, 316},
    {/* localeSet */ 506, 2, 316},
    {/* handler */ 508, 4, 320},
    {/* hostPseudoModel */ 512, 11, 312},
    {/* presence */ 523, 2, 316},
    {/* record */ 525, 2, 316},
    {/* embed */ 527, 2, 316},
    {/* version */ 529, 2, 316},
    {/* command */ 531, 3, 316},
    {/* copies */ 534, 2, 316},
    {/* staple */ 536, 3, 316},
    {/* submitFormat */ 539, 3, 316},
    {/* boolean */ 542, 4, 318},
    {/* message */ 546, 4, 316},
    {/* output */ 550, 2, 316},
    {/* psMap */ 552, 0, 316},
    {/* excludeNS */ 552, 2, 316},
    {/* assist */ 554, 3, 316},
    {/* picture */ 557, 6, 316},
    {/* traversal */ 563, 2, 316},
    {/* silentPrint */ 565, 2, 316},
    {/* webClient */ 567, 3, 316},
    {/* layoutPseudoModel */ 570, 1, 312},
    {/* producer */ 571, 2, 316},
    {/* corner */ 573, 8, 316},
    {/* msgId */ 581, 2, 316},
    {/* color */ 583, 4, 316},
    {/* keep */ 587, 5, 316},
    {/* query */ 592, 3, 316},
    {/* insert */ 595, 2, 320},
    {/* imageEdit */ 597, 3, 316},
    {/* validate */ 600, 7, 316},
    {/* digestMethods */ 607, 3, 316},
    {/* numberPatterns */ 610, 0, 316},
    {/* pageSet */ 610, 4, 317},
    {/* integer */ 614, 4, 318},
    {/* soapAddress */ 618, 2, 320},
    {/* equate */ 620, 5, 316},
    {/* formFieldFilling */ 625, 2, 316},
    {/* pageRange */ 627, 2, 316},
    {/* update */ 629, 2, 320},
    {/* connectString */ 631, 2, 320},
    {/* mode */ 633, 4, 316},
    {/* layout */ 637, 2, 316},
    {/* #xml */ 639, 1, 316},
    {/* xsdConnection */ 640, 1, 316},
    {/* traverse */ 641, 4, 316},
    {/* encodings */ 645, 3, 316},
    {/* template */ 648, 2, 319},
    {/* acrobat */ 650, 2, 316},
    {/* validationMessaging */ 652, 2, 316},
    {/* signing */ 654, 3, 316},
    {/* dataWindow */ 657, 4, 312},
    {/* script */ 661, 10, 316},
    {/* addViewerPreferences */ 671, 2, 316},
    {/* alwaysEmbed */ 673, 4, 316},
    {/* passwordEdit */ 677, 4, 316},
    {/* numericEdit */ 681, 3, 316},
    {/* encryptionMethod */ 684, 2, 316},
    {/* change */ 686, 2, 316},
    {/* pageArea */ 688, 8, 317},
    {/* submitUrl */ 696, 3, 316},
    {/* oids */ 699, 3, 316},
    {/* signature */ 702, 2, 316},
    {/* ADBE_JSConsole */ 704, 2, 316},
    {/* caption */ 706, 5, 316},
    {/* relevant */ 711, 4, 316},
    {/* flipLabel */ 715, 2, 316},
    {/* exData */ 717, 8, 318},
    {/* dayNames */ 725, 1, 316},
    {/* soapAction */ 726, 2, 320},
    {/* defaultTypeface */ 728, 3, 316},
    {/* manifest */ 731, 4, 316},
    {/* overflow */ 735, 5, 316},
    {/* linear */ 740, 3, 316},
    {/* currencySymbol */ 743, 1, 316},
    {/* delete */ 744, 2, 320},
    {/* deltas */ 746, 0, 313},
    {/* digestMethod */ 746, 2, 316},
    {/* instanceManager */ 748, 3, 316},
    {/* equateRange */ 751, 5, 316},
    {/* medium */ 756, 7, 316},
    {/* textEdit */ 763, 6, 316},
    {/* templateCache */ 769, 3, 316},
    {/* compressObjectStream */ 772, 2, 316},
    {/* dataValue */ 774, 5, 316},
    {/* accessibleContent */ 779, 2, 316},
    {/* nodeList */ 781, 0, 314},
    {/* includeXDPContent */ 781, 2, 316},
    {/* xmlConnection */ 783, 1, 316},
    {/* validateApprovalSignatures */ 784, 2, 316},
    {/* signData */ 786, 5, 316},
    {/* packets */ 791, 2, 316},
    {/* datePattern */ 793, 1, 316},
    {/* duplexOption */ 794, 2, 316},
    {/* base */ 796, 2, 316},
    {/* bind */ 798, 6, 316},
    {/* compression */ 804, 2, 316},
    {/* user */ 806, 2, 320},
    {/* rectangle */ 808, 3, 316},
    {/* effectiveOutputPolicy */ 811, 4, 316},
    {/* ADBE_JSDebugger */ 815, 2, 316},
    {/* acrobat7 */ 817, 2, 316},
    {/* interactive */ 819, 2, 316},
    {/* locale */ 821, 2, 316},
    {/* currentPage */ 823, 2, 316},
    {/* data */ 825, 2, 316},
    {/* date */ 827, 4, 318},
    {/* desc */ 831, 2, 316},
    {/* encrypt */ 833, 5, 316},
    {/* draw */ 838, 20, 317},
    {/* encryption */ 858, 2, 316},
    {/* meridiemNames */ 860, 0, 316},
    {/* messaging */ 860, 2, 316},
    {/* speak */ 862, 4, 320},
    {/* dataGroup */ 866, 0, 316},
    {/* common */ 866, 2, 316},
    {/* #text */ 868, 1, 316},
    {/* paginationOverride */ 869, 2, 316},
    {/* reasons */ 871, 3, 316},
    {/* signatureProperties */ 874, 2, 316},
    {/* threshold */ 876, 2, 316},
    {/* appearanceFilter */ 878, 4, 316},
    {/* fill */ 882, 3, 316},
    {/* font */ 885, 17, 316},
    {/* form */ 902, 1, 319},
    {/* mediumInfo */ 903, 2, 316},
    {/* certificate */ 905, 2, 320},
    {/* password */ 907, 2, 320},
    {/* runScripts */ 909, 2, 316},
    {/* trace */ 911, 2, 316},
    {/* float */ 913, 4, 318},
    {/* renderPolicy */ 917, 2, 316},
    {/* logPseudoModel */ 919, 0, 312},
    {/* destination */ 919, 2, 316},
    {/* value */ 921, 4, 316},
    {/* bookend */ 925, 4, 316},
    {/* exObject */ 929, 6, 316},
    {/* openAction */ 935, 2, 316},
    {/* neverEmbed */ 937, 4, 316},
    {/* bindItems */ 941, 3, 316},
    {/* calculate */ 944, 3, 316},
    {/* print */ 947, 2, 316},
    {/* extras */ 949, 3, 316},
    {/* proto */ 952, 0, 316},
    {/* dSigData */ 952, 0, 316},
    {/* creator */ 952, 2, 316},
    {/* connect */ 954, 7, 316},
    {/* permissions */ 961, 2, 316},
    {/* connectionSet */ 963, 0, 319},
    {/* submit */ 963, 7, 316},
    {/* range */ 970, 2, 316},
    {/* linearized */ 972, 2, 316},
    {/* packet */ 974, 1, 316},
    {/* rootElement */ 975, 2, 320},
    {/* plaintextMetadata */ 977, 4, 316},
    {/* numberSymbols */ 981, 0, 316},
    {/* printHighQuality */ 981, 2, 316},
    {/* driver */ 983, 2, 316},
    {/* incrementalLoad */ 985, 4, 316},
    {/* subjectDN */ 989, 1, 316},
    {/* compressLogicalStructure */ 990, 2, 316},
    {/* incrementalMerge */ 992, 2, 316},
    {/* radial */ 994, 3, 316},
    {/* variables */ 997, 2, 317},
    {/* timePatterns */ 999, 0, 316},
    {/* effectiveInputPolicy */ 999, 4, 316},
    {/* nameAttr */ 1003, 4, 316},
    {/* conformance */ 1007, 2, 316},
    {/* transform */ 1009, 3, 316},
    {/* lockDocument */ 1012, 4, 316},
    {/* breakAfter */ 1016, 7, 316},
    {/* line */ 1023, 4, 316},
    {/* list */ 1027, 1, 313},
    {/* source */ 1028, 3, 316},
    {/* occur */ 1031, 5, 316},
    {/* pickTrayByPDFSize */ 1036, 2, 316},
    {/* monthNames */ 1038, 1, 316},
    {/* severity */ 1039, 4, 316},
    {/* groupParent */ 1043, 2, 316},
    {/* documentAssembly */ 1045, 2, 316},
    {/* numberSymbol */ 1047, 1, 316},
    {/* tagged */ 1048, 2, 316},
    {/*  */ 1050, 5, 316},
    {/*  */ 1055, 1, -1},
    {/*  */ 1056, 1, 312},
    {/*  */ 1057, 0, 313},
    {/*  */ 1057, 8, 312},
    {/*  */ 1065, 6, 315},
    {/*  */ 1071, 0, 316},
    {/*  */ 1071, 0, 316},
    {/*  */ 1071, 2, 316},
    {/*  */ 1073, 2, 316},
};
const int32_t g_iScriptIndexCount =
    sizeof(g_XFAScriptIndex) / sizeof(XFA_SCRIPTHIERARCHY);
const XFA_SCRIPTATTRIBUTEINFO g_SomAttributeData[] = {
    /* ps */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* to */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* ui */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Ui::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Ui::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* recordSet */
    {0xb3543a6, L"max", (XFA_ATTRIBUTE_CALLBACK)&CJX_RecordSet::max,
     XFA_Attribute::Max, XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_RecordSet::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x45a6daf8, L"eofAction",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_RecordSet::eofAction,
     XFA_Attribute::EofAction, XFA_ScriptType::Basic},
    {0x5ec958c0, L"cursorType",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_RecordSet::cursorType,
     XFA_Attribute::CursorType, XFA_ScriptType::Basic},
    {0x79975f2b, L"lockType", (XFA_ATTRIBUTE_CALLBACK)&CJX_RecordSet::lockType,
     XFA_Attribute::LockType, XFA_ScriptType::Basic},
    {0xa5340ff5, L"bofAction",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_RecordSet::bofAction,
     XFA_Attribute::BofAction, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_RecordSet::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xc5762157, L"cursorLocation",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_RecordSet::cursorLocation,
     XFA_Attribute::CursorLocation, XFA_ScriptType::Basic},

    /* subsetBelow */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* subformSet */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_SubformSet::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x1ee2d24d, L"instanceIndex",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_SubformSet::instanceIndex,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x8c99377e, L"relation", (XFA_ATTRIBUTE_CALLBACK)&CJX_SubformSet::relation,
     XFA_Attribute::Relation, XFA_ScriptType::Basic},
    {0x8e1c2921, L"relevant", (XFA_ATTRIBUTE_CALLBACK)&CJX_SubformSet::relevant,
     XFA_Attribute::Relevant, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_SubformSet::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* adobeExtensionLevel */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* typeface */
    {0x31b19c1, L"name",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Name, XFA_ScriptType::Basic},

    /* break */
    {0x3106c3a, L"beforeTarget",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Break::beforeTarget,
     XFA_Attribute::BeforeTarget, XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Break::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x13a08bdb, L"overflowTarget",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Break::overflowTarget,
     XFA_Attribute::OverflowTarget, XFA_ScriptType::Basic},
    {0x169134a1, L"overflowLeader",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Break::overflowLeader,
     XFA_Attribute::OverflowLeader, XFA_ScriptType::Basic},
    {0x20914367, L"overflowTrailer",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Break::overflowTrailer,
     XFA_Attribute::OverflowTrailer, XFA_ScriptType::Basic},
    {0x453eaf38, L"startNew", (XFA_ATTRIBUTE_CALLBACK)&CJX_Break::startNew,
     XFA_Attribute::StartNew, XFA_ScriptType::Basic},
    {0x64110ab5, L"bookendTrailer",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Break::bookendTrailer,
     XFA_Attribute::BookendTrailer, XFA_ScriptType::Basic},
    {0xb6b44172, L"after", (XFA_ATTRIBUTE_CALLBACK)&CJX_Break::after,
     XFA_Attribute::After, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Break::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xc3c1442f, L"bookendLeader",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Break::bookendLeader,
     XFA_Attribute::BookendLeader, XFA_ScriptType::Basic},
    {0xcb150479, L"afterTarget",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Break::afterTarget,
     XFA_Attribute::AfterTarget, XFA_ScriptType::Basic},
    {0xf4ffce73, L"before", (XFA_ATTRIBUTE_CALLBACK)&CJX_Break::before,
     XFA_Attribute::Before, XFA_ScriptType::Basic},

    /* fontInfo */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* numberPattern */
    {0x31b19c1, L"name",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Name, XFA_ScriptType::Basic},

    /* dynamicRender */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xd6e27f1d, L"value",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Som_DefaultValue_Read,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* printScaling */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* checkButton */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_CheckButton::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x47cfa43a, L"allowNeutral",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_CheckButton::allowNeutral,
     XFA_Attribute::AllowNeutral, XFA_ScriptType::Basic},
    {0x7c2fd80b, L"mark", (XFA_ATTRIBUTE_CALLBACK)&CJX_CheckButton::mark,
     XFA_Attribute::Mark, XFA_ScriptType::Basic},
    {0x8ed182d1, L"shape", (XFA_ATTRIBUTE_CALLBACK)&CJX_CheckButton::shape,
     XFA_Attribute::Shape, XFA_ScriptType::Basic},
    {0xa686975b, L"size", (XFA_ATTRIBUTE_CALLBACK)&CJX_CheckButton::size,
     XFA_Attribute::Size, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_CheckButton::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* datePatterns */

    /* sourceSet */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_SourceSet::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_SourceSet::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* amd */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* arc */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Arc::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x5c054755, L"startAngle", (XFA_ATTRIBUTE_CALLBACK)&CJX_Arc::startAngle,
     XFA_Attribute::StartAngle, XFA_ScriptType::Basic},
    {0x74788f8b, L"sweepAngle", (XFA_ATTRIBUTE_CALLBACK)&CJX_Arc::sweepAngle,
     XFA_Attribute::SweepAngle, XFA_ScriptType::Basic},
    {0x9d833d75, L"circular", (XFA_ATTRIBUTE_CALLBACK)&CJX_Arc::circular,
     XFA_Attribute::Circular, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Arc::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xd996fa9b, L"hand", (XFA_ATTRIBUTE_CALLBACK)&CJX_Arc::hand,
     XFA_Attribute::Hand, XFA_ScriptType::Basic},

    /* day */

    /* era */

    /* jog */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* log */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* map */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Map::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xb0e5485d, L"bind", (XFA_ATTRIBUTE_CALLBACK)&CJX_Map::bind,
     XFA_Attribute::Bind, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Map::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xcd7f7b54, L"from", (XFA_ATTRIBUTE_CALLBACK)&CJX_Map::from,
     XFA_Attribute::From, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* mdp */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Mdp::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x8e29d794, L"signatureType",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Mdp::signatureType,
     XFA_Attribute::SignatureType, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Mdp::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xe11a2cbc, L"permissions", (XFA_ATTRIBUTE_CALLBACK)&CJX_Mdp::permissions,
     XFA_Attribute::Permissions, XFA_ScriptType::Basic},

    /* breakBefore */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_BreakBefore::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x453eaf38, L"startNew",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_BreakBefore::startNew,
     XFA_Attribute::StartNew, XFA_ScriptType::Basic},
    {0x9dcc3ab3, L"trailer", (XFA_ATTRIBUTE_CALLBACK)&CJX_BreakBefore::trailer,
     XFA_Attribute::Trailer, XFA_ScriptType::Basic},
    {0xa6118c89, L"targetType",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_BreakBefore::targetType,
     XFA_Attribute::TargetType, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_BreakBefore::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xc8da4da7, L"target", (XFA_ATTRIBUTE_CALLBACK)&CJX_BreakBefore::target,
     XFA_Attribute::Target, XFA_ScriptType::Basic},
    {0xcbcaf66d, L"leader", (XFA_ATTRIBUTE_CALLBACK)&CJX_BreakBefore::leader,
     XFA_Attribute::Leader, XFA_ScriptType::Basic},

    /* oid */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Oid::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Oid::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* pcl */
    {0x31b19c1, L"name",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Name, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* pdf */
    {0x31b19c1, L"name",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Name, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* ref */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Ref::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Ref::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* uri */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Uri::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Uri::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* xdc */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* xdp */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* xfa */
    {0x2d574d58, L"this", (XFA_ATTRIBUTE_CALLBACK)&CJX_Xfa::thisValue,
     XFA_Attribute::Unknown, XFA_ScriptType::Object},
    {0x4fdc3454, L"timeStamp", (XFA_ATTRIBUTE_CALLBACK)&CJX_Xfa::timeStamp,
     XFA_Attribute::TimeStamp, XFA_ScriptType::Basic},
    {0xb598a1f7, L"uuid", (XFA_ATTRIBUTE_CALLBACK)&CJX_Xfa::uuid,
     XFA_Attribute::Uuid, XFA_ScriptType::Basic},

    /* xsl */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* zpl */
    {0x31b19c1, L"name",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Name, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* cache */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* margin */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Margin::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xcfea02e, L"leftInset", (XFA_ATTRIBUTE_CALLBACK)&CJX_Margin::leftInset,
     XFA_Attribute::LeftInset, XFA_ScriptType::Basic},
    {0x1356caf8, L"bottomInset",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Margin::bottomInset,
     XFA_Attribute::BottomInset, XFA_ScriptType::Basic},
    {0x25764436, L"topInset", (XFA_ATTRIBUTE_CALLBACK)&CJX_Margin::topInset,
     XFA_Attribute::TopInset, XFA_ScriptType::Basic},
    {0x8a692521, L"rightInset", (XFA_ATTRIBUTE_CALLBACK)&CJX_Margin::rightInset,
     XFA_Attribute::RightInset, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Margin::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* keyUsage */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_KeyUsage::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x1e459b8f, L"nonRepudiation",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_KeyUsage::nonRepudiation,
     XFA_Attribute::NonRepudiation, XFA_ScriptType::Basic},
    {0x2bb3f470, L"encipherOnly",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_KeyUsage::encipherOnly,
     XFA_Attribute::EncipherOnly, XFA_ScriptType::Basic},
    {0x2f16a382, L"type", (XFA_ATTRIBUTE_CALLBACK)&CJX_KeyUsage::type,
     XFA_Attribute::Type, XFA_ScriptType::Basic},
    {0x5f760b50, L"digitalSignature",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_KeyUsage::digitalSignature,
     XFA_Attribute::DigitalSignature, XFA_ScriptType::Basic},
    {0x69aa2292, L"crlSign", (XFA_ATTRIBUTE_CALLBACK)&CJX_KeyUsage::crlSign,
     XFA_Attribute::CrlSign, XFA_ScriptType::Basic},
    {0x98fd4d81, L"keyAgreement",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_KeyUsage::keyAgreement,
     XFA_Attribute::KeyAgreement, XFA_ScriptType::Basic},
    {0xa66404cb, L"keyEncipherment",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_KeyUsage::keyEncipherment,
     XFA_Attribute::KeyEncipherment, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_KeyUsage::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xca5dc27c, L"dataEncipherment",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_KeyUsage::dataEncipherment,
     XFA_Attribute::DataEncipherment, XFA_ScriptType::Basic},
    {0xe8f118a8, L"keyCertSign",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_KeyUsage::keyCertSign,
     XFA_Attribute::KeyCertSign, XFA_ScriptType::Basic},
    {0xfea53ec6, L"decipherOnly",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_KeyUsage::decipherOnly,
     XFA_Attribute::DecipherOnly, XFA_ScriptType::Basic},

    /* exclude */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* choiceList */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_ChoiceList::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x8b90e1f2, L"open", (XFA_ATTRIBUTE_CALLBACK)&CJX_ChoiceList::open,
     XFA_Attribute::Open, XFA_ScriptType::Basic},
    {0x957fa006, L"commitOn", (XFA_ATTRIBUTE_CALLBACK)&CJX_ChoiceList::commitOn,
     XFA_Attribute::CommitOn, XFA_ScriptType::Basic},
    {0xb12128b7, L"textEntry",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_ChoiceList::textEntry,
     XFA_Attribute::TextEntry, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_ChoiceList::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* level */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* labelPrinter */
    {0x31b19c1, L"name",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Name, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* calendarSymbols */
    {0x31b19c1, L"name",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Name, XFA_ScriptType::Basic},

    /* para */
    {0x2282c73, L"hAlign", (XFA_ATTRIBUTE_CALLBACK)&CJX_Para::hAlign,
     XFA_Attribute::HAlign, XFA_ScriptType::Basic},
    {0x8d4f1c7, L"textIndent", (XFA_ATTRIBUTE_CALLBACK)&CJX_Para::textIndent,
     XFA_Attribute::TextIndent, XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Para::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x2a82d99c, L"marginRight", (XFA_ATTRIBUTE_CALLBACK)&CJX_Para::marginRight,
     XFA_Attribute::MarginRight, XFA_ScriptType::Basic},
    {0x534729c9, L"marginLeft", (XFA_ATTRIBUTE_CALLBACK)&CJX_Para::marginLeft,
     XFA_Attribute::MarginLeft, XFA_ScriptType::Basic},
    {0x5739d1ff, L"radixOffset", (XFA_ATTRIBUTE_CALLBACK)&CJX_Para::radixOffset,
     XFA_Attribute::RadixOffset, XFA_ScriptType::Basic},
    {0x577682ac, L"preserve", (XFA_ATTRIBUTE_CALLBACK)&CJX_Para::preserve,
     XFA_Attribute::Preserve, XFA_ScriptType::Basic},
    {0x731e0665, L"spaceBelow", (XFA_ATTRIBUTE_CALLBACK)&CJX_Para::spaceBelow,
     XFA_Attribute::SpaceBelow, XFA_ScriptType::Basic},
    {0x7a7cc341, L"vAlign", (XFA_ATTRIBUTE_CALLBACK)&CJX_Para::vAlign,
     XFA_Attribute::VAlign, XFA_ScriptType::Basic},
    {0x836d4d7c, L"tabDefault", (XFA_ATTRIBUTE_CALLBACK)&CJX_Para::tabDefault,
     XFA_Attribute::TabDefault, XFA_ScriptType::Basic},
    {0x8fa01790, L"tabStops", (XFA_ATTRIBUTE_CALLBACK)&CJX_Para::tabStops,
     XFA_Attribute::TabStops, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Para::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xd4b01921, L"lineHeight", (XFA_ATTRIBUTE_CALLBACK)&CJX_Para::lineHeight,
     XFA_Attribute::LineHeight, XFA_ScriptType::Basic},
    {0xe18b5659, L"spaceAbove", (XFA_ATTRIBUTE_CALLBACK)&CJX_Para::spaceAbove,
     XFA_Attribute::SpaceAbove, XFA_ScriptType::Basic},

    /* part */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* pdfa */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* filter */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Filter::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Filter::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xd861f8af, L"addRevocationInfo",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Filter::addRevocationInfo,
     XFA_Attribute::AddRevocationInfo, XFA_ScriptType::Basic},

    /* present */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* pagination */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* encoding */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Encoding::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Encoding::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* event */
    {0xbb8df5d, L"ref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Event::ref,
     XFA_Attribute::Ref, XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Event::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Event::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xd6128d8d, L"activity", (XFA_ATTRIBUTE_CALLBACK)&CJX_Event::activity,
     XFA_Attribute::Activity, XFA_ScriptType::Basic},

    /* whitespace */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* defaultUi */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_DefaultUi::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_DefaultUi::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* dataModel */

    /* barcode */
    {0x43e349b, L"dataRowCount",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Barcode::dataRowCount,
     XFA_Attribute::DataRowCount, XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Barcode::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x28e17e91, L"dataPrep", (XFA_ATTRIBUTE_CALLBACK)&CJX_Barcode::dataPrep,
     XFA_Attribute::DataPrep, XFA_ScriptType::Basic},
    {0x2f16a382, L"type", (XFA_ATTRIBUTE_CALLBACK)&CJX_Barcode::type,
     XFA_Attribute::Type, XFA_ScriptType::Basic},
    {0x3650557e, L"textLocation",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Barcode::textLocation,
     XFA_Attribute::TextLocation, XFA_ScriptType::Basic},
    {0x3b582286, L"moduleWidth",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Barcode::moduleWidth,
     XFA_Attribute::ModuleWidth, XFA_ScriptType::Basic},
    {0x52666f1c, L"printCheckDigit",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Barcode::printCheckDigit,
     XFA_Attribute::PrintCheckDigit, XFA_ScriptType::Basic},
    {0x5404d6df, L"moduleHeight",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Barcode::moduleHeight,
     XFA_Attribute::ModuleHeight, XFA_ScriptType::Basic},
    {0x5ab23b6c, L"startChar", (XFA_ATTRIBUTE_CALLBACK)&CJX_Barcode::startChar,
     XFA_Attribute::StartChar, XFA_ScriptType::Basic},
    {0x7c732a66, L"truncate", (XFA_ATTRIBUTE_CALLBACK)&CJX_Barcode::truncate,
     XFA_Attribute::Truncate, XFA_ScriptType::Basic},
    {0x8d181d61, L"wideNarrowRatio",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Barcode::wideNarrowRatio,
     XFA_Attribute::WideNarrowRatio, XFA_ScriptType::Basic},
    {0x99800d7a, L"errorCorrectionLevel",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Barcode::errorCorrectionLevel,
     XFA_Attribute::ErrorCorrectionLevel, XFA_ScriptType::Basic},
    {0x9a63da3d, L"upsMode", (XFA_ATTRIBUTE_CALLBACK)&CJX_Barcode::upsMode,
     XFA_Attribute::UpsMode, XFA_ScriptType::Basic},
    {0xaf754613, L"checksum", (XFA_ATTRIBUTE_CALLBACK)&CJX_Barcode::checksum,
     XFA_Attribute::Checksum, XFA_ScriptType::Basic},
    {0xb045fbc5, L"charEncoding",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Barcode::charEncoding,
     XFA_Attribute::CharEncoding, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Barcode::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xc035c6b1, L"dataColumnCount",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Barcode::dataColumnCount,
     XFA_Attribute::DataColumnCount, XFA_ScriptType::Basic},
    {0xd3c84d25, L"rowColumnRatio",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Barcode::rowColumnRatio,
     XFA_Attribute::RowColumnRatio, XFA_ScriptType::Basic},
    {0xd57c513c, L"dataLength",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Barcode::dataLength,
     XFA_Attribute::DataLength, XFA_ScriptType::Basic},
    {0xf575ca75, L"endChar", (XFA_ATTRIBUTE_CALLBACK)&CJX_Barcode::endChar,
     XFA_Attribute::EndChar, XFA_ScriptType::Basic},

    /* timePattern */
    {0x31b19c1, L"name",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Name, XFA_ScriptType::Basic},

    /* batchOutput */
    {0x28dee6e9, L"format",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Format, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* enforce */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* currencySymbols */

    /* addSilentPrint */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* rename */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* operation */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Operation::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x60d4c8b1, L"output", (XFA_ATTRIBUTE_CALLBACK)&CJX_Operation::output,
     XFA_Attribute::Output, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Operation::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xd6a39990, L"input", (XFA_ATTRIBUTE_CALLBACK)&CJX_Operation::input,
     XFA_Attribute::Input, XFA_ScriptType::Basic},

    /* typefaces */

    /* subjectDNs */
    {0x2f16a382, L"type", (XFA_ATTRIBUTE_CALLBACK)&CJX_SubjectDNs::type,
     XFA_Attribute::Type, XFA_ScriptType::Basic},

    /* issuers */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Issuers::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x2f16a382, L"type", (XFA_ATTRIBUTE_CALLBACK)&CJX_Issuers::type,
     XFA_Attribute::Type, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Issuers::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* signaturePseudoModel */

    /* wsdlConnection */
    {0x2b5df51e, L"dataDescription",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_WsdlConnection::dataDescription,
     XFA_Attribute::DataDescription, XFA_ScriptType::Basic},

    /* debug */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* delta */
    {0x6c0d9600, L"currentValue",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Delta::currentValue, XFA_Attribute::Unknown,
     XFA_ScriptType::Basic},
    {0x942643f0, L"savedValue", (XFA_ATTRIBUTE_CALLBACK)&CJX_Delta::savedValue,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xc8da4da7, L"target", (XFA_ATTRIBUTE_CALLBACK)&CJX_Delta::target,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},

    /* eraNames */

    /* modifyAnnots */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* startNode */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* button */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Button::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Button::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xd4cc53f8, L"highlight", (XFA_ATTRIBUTE_CALLBACK)&CJX_Button::highlight,
     XFA_Attribute::Highlight, XFA_ScriptType::Basic},

    /* format */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Format::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Format::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* border */
    {0x5518c25, L"break", (XFA_ATTRIBUTE_CALLBACK)&CJX_Border::breakValue,
     XFA_Attribute::Break, XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Border::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x570ce835, L"presence", (XFA_ATTRIBUTE_CALLBACK)&CJX_Border::presence,
     XFA_Attribute::Presence, XFA_ScriptType::Basic},
    {0x8e1c2921, L"relevant", (XFA_ATTRIBUTE_CALLBACK)&CJX_Border::relevant,
     XFA_Attribute::Relevant, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Border::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xd996fa9b, L"hand", (XFA_ATTRIBUTE_CALLBACK)&CJX_Border::hand,
     XFA_Attribute::Hand, XFA_ScriptType::Basic},

    /* area */
    {0x78, L"x", (XFA_ATTRIBUTE_CALLBACK)&CJX_Area::x, XFA_Attribute::X,
     XFA_ScriptType::Basic},
    {0x79, L"y", (XFA_ATTRIBUTE_CALLBACK)&CJX_Area::y, XFA_Attribute::Y,
     XFA_ScriptType::Basic},
    {0x21aed, L"id", (XFA_ATTRIBUTE_CALLBACK)&CJX_Area::id, XFA_Attribute::Id,
     XFA_ScriptType::Basic},
    {0x31b19c1, L"name", (XFA_ATTRIBUTE_CALLBACK)&CJX_Area::name,
     XFA_Attribute::Name, XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Area::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x1059ec18, L"level",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_Integer,
     XFA_Attribute::Level, XFA_ScriptType::Basic},
    {0x8e1c2921, L"relevant", (XFA_ATTRIBUTE_CALLBACK)&CJX_Area::relevant,
     XFA_Attribute::Relevant, XFA_ScriptType::Basic},
    {0xac06e2b0, L"colSpan", (XFA_ATTRIBUTE_CALLBACK)&CJX_Area::colSpan,
     XFA_Attribute::ColSpan, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Area::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* hyphenation */
    {0x21aed, L"id",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Id, XFA_ScriptType::Basic},
    {0xc0811ed, L"use",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x2f105f72, L"wordCharacterCount",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::WordCharacterCount, XFA_ScriptType::Basic},
    {0x3d123c26, L"hyphenate",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Hyphenate, XFA_ScriptType::Basic},
    {0x66539c48, L"excludeInitialCap",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::ExcludeInitialCap, XFA_ScriptType::Basic},
    {0x6a95c976, L"pushCharacterCount",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::PushCharacterCount, XFA_ScriptType::Basic},
    {0x982bd892, L"remainCharacterCount",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::RemainCharacterCount, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xe5c96d6a, L"excludeAllCaps",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::ExcludeAllCaps, XFA_ScriptType::Basic},

    /* text */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Text::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x8af2e657, L"maxChars", (XFA_ATTRIBUTE_CALLBACK)&CJX_Text::maxChars,
     XFA_Attribute::MaxChars, XFA_ScriptType::Basic},
    {0xa52682bd, L"{default}", (XFA_ATTRIBUTE_CALLBACK)&CJX_Text::defaultValue,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Text::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xd6e27f1d, L"value", (XFA_ATTRIBUTE_CALLBACK)&CJX_Text::value,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},

    /* time */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Time::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xa52682bd, L"{default}", (XFA_ATTRIBUTE_CALLBACK)&CJX_Time::defaultValue,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Time::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xd6e27f1d, L"value", (XFA_ATTRIBUTE_CALLBACK)&CJX_Time::value,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},

    /* type */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* overprint */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* certificates */
    {0xc080cd3, L"url", (XFA_ATTRIBUTE_CALLBACK)&CJX_Certificates::url,
     XFA_Attribute::Url, XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Certificates::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xa6710262, L"credentialServerPolicy",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Certificates::credentialServerPolicy,
     XFA_Attribute::CredentialServerPolicy, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Certificates::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xc2ba0923, L"urlPolicy",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Certificates::urlPolicy,
     XFA_Attribute::UrlPolicy, XFA_ScriptType::Basic},

    /* encryptionMethods */
    {0xc0811ed, L"use",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x2f16a382, L"type",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Type, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* setProperty */
    {0x47d03490, L"connection",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_SetProperty::connection,
     XFA_Attribute::Connection, XFA_ScriptType::Basic},
    {0xc8da4da7, L"target", (XFA_ATTRIBUTE_CALLBACK)&CJX_SetProperty::target,
     XFA_Attribute::Target, XFA_ScriptType::Basic},

    /* printerName */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* startPage */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* pageOffset */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* dateTime */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_DateTime::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xa52682bd, L"{default}",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_DateTime::defaultValue,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_DateTime::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xd6e27f1d, L"value", (XFA_ATTRIBUTE_CALLBACK)&CJX_DateTime::value,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},

    /* comb */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Comb::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x78bff531, L"numberOfCells",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Comb::numberOfCells,
     XFA_Attribute::NumberOfCells, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Comb::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* pattern */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Pattern::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x2f16a382, L"type", (XFA_ATTRIBUTE_CALLBACK)&CJX_Pattern::type,
     XFA_Attribute::Type, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Pattern::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* ifEmpty */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* suppressBanner */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* outputBin */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* field */
    {0x68, L"h", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::h, XFA_Attribute::H,
     XFA_ScriptType::Basic},
    {0x77, L"w", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::w, XFA_Attribute::W,
     XFA_ScriptType::Basic},
    {0x78, L"x", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::x, XFA_Attribute::X,
     XFA_ScriptType::Basic},
    {0x79, L"y", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::y, XFA_Attribute::Y,
     XFA_ScriptType::Basic},
    {0x2282c73, L"hAlign", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::hAlign,
     XFA_Attribute::HAlign, XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x1abbd7e0, L"dataNode",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Som_DataNode,
     XFA_Attribute::Unknown, XFA_ScriptType::Object},
    {0x25839852, L"access", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::access,
     XFA_Attribute::Access, XFA_ScriptType::Basic},
    {0x2ee7678f, L"rotate", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::rotate,
     XFA_Attribute::Rotate, XFA_ScriptType::Basic},
    {0x3b1ddd06, L"fillColor", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::fillColor,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x54c399e3, L"formattedValue",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::formattedValue, XFA_Attribute::Unknown,
     XFA_ScriptType::Basic},
    {0x570ce835, L"presence", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::presence,
     XFA_Attribute::Presence, XFA_ScriptType::Basic},
    {0x5a3b375d, L"borderColor",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::borderColor, XFA_Attribute::Unknown,
     XFA_ScriptType::Basic},
    {0x5e936ed6, L"fontColor", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::fontColor,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x6826c408, L"parentSubform",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::parentSubform, XFA_Attribute::Unknown,
     XFA_ScriptType::Basic},
    {0x79b67434, L"mandatoryMessage",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::mandatoryMessage,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x7a7cc341, L"vAlign", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::vAlign,
     XFA_Attribute::VAlign, XFA_ScriptType::Basic},
    {0x7c2ff6ae, L"maxH", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::maxH,
     XFA_Attribute::MaxH, XFA_ScriptType::Basic},
    {0x7c2ff6bd, L"maxW", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::maxW,
     XFA_Attribute::MaxW, XFA_ScriptType::Basic},
    {0x7d02356c, L"minH", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::minH,
     XFA_Attribute::MinH, XFA_ScriptType::Basic},
    {0x7d02357b, L"minW", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::minW,
     XFA_Attribute::MinW, XFA_ScriptType::Basic},
    {0x85fd6faf, L"mandatory", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::mandatory,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x8e1c2921, L"relevant", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::relevant,
     XFA_Attribute::Relevant, XFA_ScriptType::Basic},
    {0x964fb42e, L"formatMessage",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::formatMessage, XFA_Attribute::Unknown,
     XFA_ScriptType::Basic},
    {0xa03cf627, L"rawValue", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::rawValue,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xa52682bd, L"{default}", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::defaultValue,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xa60dd202, L"length",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Field_Length,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xac06e2b0, L"colSpan", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::colSpan,
     XFA_Attribute::ColSpan, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xbc8fa350, L"locale", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::locale,
     XFA_Attribute::Locale, XFA_ScriptType::Basic},
    {0xc2bd40fd, L"anchorType", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::anchorType,
     XFA_Attribute::AnchorType, XFA_ScriptType::Basic},
    {0xc4fed09b, L"accessKey", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::accessKey,
     XFA_Attribute::AccessKey, XFA_ScriptType::Basic},
    {0xcabfa3d0, L"validationMessage",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::validationMessage,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xdcecd663, L"editValue", (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::editValue,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xe07e5061, L"selectedIndex",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::selectedIndex, XFA_Attribute::Unknown,
     XFA_ScriptType::Basic},
    {0xf65e34be, L"borderWidth",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Field::borderWidth, XFA_Attribute::Unknown,
     XFA_ScriptType::Basic},

    /* agent */
    {0x31b19c1, L"name",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Name, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* outputXSL */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* adjustData */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* autoSave */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* contentArea */
    {0x68, L"h", (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::H, XFA_ScriptType::Basic},
    {0x77, L"w", (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::W, XFA_ScriptType::Basic},
    {0x78, L"x", (XFA_ATTRIBUTE_CALLBACK)&CJX_ContentArea::x, XFA_Attribute::X,
     XFA_ScriptType::Basic},
    {0x79, L"y", (XFA_ATTRIBUTE_CALLBACK)&CJX_ContentArea::y, XFA_Attribute::Y,
     XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_ContentArea::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x8e1c2921, L"relevant",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_ContentArea::relevant,
     XFA_Attribute::Relevant, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_ContentArea::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* eventPseudoModel */
    {0xd843798, L"fullText",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_EventPseudoModel::fullText,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x1b6d1cf5, L"reenter",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_EventPseudoModel::reenter,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x1e6ffa9a, L"prevContentType",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_EventPseudoModel::prevContentType,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x25a3c206, L"soapFaultString",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_EventPseudoModel::soapFaultString,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x2e00c007, L"newContentType",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_EventPseudoModel::newContentType,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x4570500f, L"modifier",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_EventPseudoModel::modifier,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x50e2e33b, L"selEnd",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_EventPseudoModel::selEnd,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x57de87c2, L"prevText",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_EventPseudoModel::prevText,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x6ea04e0a, L"soapFaultCode",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_EventPseudoModel::soapFaultCode,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x6f6556cf, L"newText",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_EventPseudoModel::newText,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x891f4606, L"change",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_EventPseudoModel::change,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x8fa3c19e, L"shift", (XFA_ATTRIBUTE_CALLBACK)&CJX_EventPseudoModel::shift,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xa9d9b2e1, L"keyDown",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_EventPseudoModel::keyDown,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xbfc89db2, L"selStart",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_EventPseudoModel::selStart,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xc32a5812, L"commitKey",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_EventPseudoModel::commitKey,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xc8da4da7, L"target",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_EventPseudoModel::target,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},

    /* wsdlAddress */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_WsdlAddress::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_WsdlAddress::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* solid */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Solid::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Solid::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* dateTimeSymbols */

    /* encryptionLevel */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* edge */
    {0xa2e3514, L"cap", (XFA_ATTRIBUTE_CALLBACK)&CJX_Edge::cap,
     XFA_Attribute::Cap, XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Edge::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x5392ea58, L"stroke", (XFA_ATTRIBUTE_CALLBACK)&CJX_Edge::stroke,
     XFA_Attribute::Stroke, XFA_ScriptType::Basic},
    {0x570ce835, L"presence", (XFA_ATTRIBUTE_CALLBACK)&CJX_Edge::presence,
     XFA_Attribute::Presence, XFA_ScriptType::Basic},
    {0x94446dcc, L"thickness", (XFA_ATTRIBUTE_CALLBACK)&CJX_Edge::thickness,
     XFA_Attribute::Thickness, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Edge::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* stipple */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Stipple::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x1ec8ab2c, L"rate", (XFA_ATTRIBUTE_CALLBACK)&CJX_Stipple::rate,
     XFA_Attribute::Rate, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Stipple::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* attributes */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* versionControl */
    {0x7b29630a, L"sourceBelow",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::SourceBelow, XFA_ScriptType::Basic},
    {0x8fc36c0a, L"outputBelow",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::OutputBelow, XFA_ScriptType::Basic},
    {0xe996b2fe, L"sourceAbove",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::SourceAbove, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* meridiem */

    /* exclGroup */
    {0x68, L"h", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::h, XFA_Attribute::H,
     XFA_ScriptType::Basic},
    {0x77, L"w", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::w, XFA_Attribute::W,
     XFA_ScriptType::Basic},
    {0x78, L"x", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::x, XFA_Attribute::X,
     XFA_ScriptType::Basic},
    {0x79, L"y", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::y, XFA_Attribute::Y,
     XFA_ScriptType::Basic},
    {0x2282c73, L"hAlign", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::hAlign,
     XFA_Attribute::HAlign, XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xf23332f, L"errorText",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_ExclGroup_ErrorText,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x1abbd7e0, L"dataNode",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Som_DataNode,
     XFA_Attribute::Unknown, XFA_ScriptType::Object},
    {0x25839852, L"access", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::access,
     XFA_Attribute::Access, XFA_ScriptType::Basic},
    {0x3b1ddd06, L"fillColor",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::fillColor, XFA_Attribute::Unknown,
     XFA_ScriptType::Basic},
    {0x570ce835, L"presence", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::presence,
     XFA_Attribute::Presence, XFA_ScriptType::Basic},
    {0x5a3b375d, L"borderColor",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::borderColor,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x79b67434, L"mandatoryMessage",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::mandatoryMessage,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x7a7cc341, L"vAlign", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::vAlign,
     XFA_Attribute::VAlign, XFA_ScriptType::Basic},
    {0x7c2ff6ae, L"maxH", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::maxH,
     XFA_Attribute::MaxH, XFA_ScriptType::Basic},
    {0x7c2ff6bd, L"maxW", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::maxW,
     XFA_Attribute::MaxW, XFA_ScriptType::Basic},
    {0x7d02356c, L"minH", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::minH,
     XFA_Attribute::MinH, XFA_ScriptType::Basic},
    {0x7d02357b, L"minW", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::minW,
     XFA_Attribute::MinW, XFA_ScriptType::Basic},
    {0x7e7e845e, L"layout", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::layout,
     XFA_Attribute::Layout, XFA_ScriptType::Basic},
    {0x846599f8, L"transient",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::transient, XFA_Attribute::Unknown,
     XFA_ScriptType::Basic},
    {0x85fd6faf, L"mandatory",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::mandatory, XFA_Attribute::Unknown,
     XFA_ScriptType::Basic},
    {0x8e1c2921, L"relevant", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::relevant,
     XFA_Attribute::Relevant, XFA_ScriptType::Basic},
    {0xa03cf627, L"rawValue", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::rawValue,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xa52682bd, L"{default}",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::defaultValue,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xac06e2b0, L"colSpan", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::colSpan,
     XFA_Attribute::ColSpan, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xc2bd40fd, L"anchorType",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::anchorType,
     XFA_Attribute::AnchorType, XFA_ScriptType::Basic},
    {0xc4fed09b, L"accessKey",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::accessKey,
     XFA_Attribute::AccessKey, XFA_ScriptType::Basic},
    {0xcabfa3d0, L"validationMessage",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::validationMessage,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xf65e34be, L"borderWidth",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_ExclGroup::borderWidth,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},

    /* toolTip */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_ToolTip::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_ToolTip::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* compress */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xeda9017a, L"scope",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Scope, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* reason */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Reason::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Reason::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* execute */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Execute::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x47d03490, L"connection",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Execute::connection,
     XFA_Attribute::Connection, XFA_ScriptType::Basic},
    {0x6cfa828a, L"runAt", (XFA_ATTRIBUTE_CALLBACK)&CJX_Execute::runAt,
     XFA_Attribute::RunAt, XFA_ScriptType::Basic},
    {0xa1b0d2f5, L"executeType",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Execute::executeType,
     XFA_Attribute::ExecuteType, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Execute::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* contentCopy */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* dateTimeEdit */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_DateTimeEdit::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_DateTimeEdit::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xe6f99487, L"hScrollPolicy",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_DateTimeEdit::hScrollPolicy,
     XFA_Attribute::HScrollPolicy, XFA_ScriptType::Basic},

    /* config */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* image */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Image::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x42fed1fd, L"contentType",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Image::contentType,
     XFA_Attribute::ContentType, XFA_ScriptType::Basic},
    {0x54fa722c, L"transferEncoding",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Image::transferEncoding,
     XFA_Attribute::TransferEncoding, XFA_ScriptType::Basic},
    {0xa52682bd, L"{default}", (XFA_ATTRIBUTE_CALLBACK)&CJX_Image::defaultValue,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Image::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xd171b240, L"aspect", (XFA_ATTRIBUTE_CALLBACK)&CJX_Image::aspect,
     XFA_Attribute::Aspect, XFA_ScriptType::Basic},
    {0xd6e27f1d, L"value", (XFA_ATTRIBUTE_CALLBACK)&CJX_Image::value,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xdb55fec5, L"href", (XFA_ATTRIBUTE_CALLBACK)&CJX_Image::href,
     XFA_Attribute::Href, XFA_ScriptType::Basic},

    /* #xHTML */
    {0xd6e27f1d, L"value",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Value, XFA_ScriptType::Basic},

    /* numberOfCopies */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* behaviorOverride */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* timeStamp */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_TimeStamp::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x2f16a382, L"type", (XFA_ATTRIBUTE_CALLBACK)&CJX_TimeStamp::type,
     XFA_Attribute::Type, XFA_ScriptType::Basic},
    {0x7f6fd3d7, L"server", (XFA_ATTRIBUTE_CALLBACK)&CJX_TimeStamp::server,
     XFA_Attribute::Server, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_TimeStamp::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* month */

    /* viewerPreferences */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* scriptModel */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* decimal */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Decimal::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x4b8bc840, L"fracDigits",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Decimal::fracDigits,
     XFA_Attribute::FracDigits, XFA_ScriptType::Basic},
    {0xa52682bd, L"{default}",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Decimal::defaultValue, XFA_Attribute::Unknown,
     XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Decimal::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xd6e27f1d, L"value", (XFA_ATTRIBUTE_CALLBACK)&CJX_Decimal::value,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xde7f92ba, L"leadDigits",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Decimal::leadDigits,
     XFA_Attribute::LeadDigits, XFA_ScriptType::Basic},

    /* subform */
    {0x68, L"h", (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::h, XFA_Attribute::H,
     XFA_ScriptType::Basic},
    {0x77, L"w", (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::w, XFA_Attribute::W,
     XFA_ScriptType::Basic},
    {0x78, L"x", (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::x, XFA_Attribute::X,
     XFA_ScriptType::Basic},
    {0x79, L"y", (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::y, XFA_Attribute::Y,
     XFA_ScriptType::Basic},
    {0x2282c73, L"hAlign", (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::hAlign,
     XFA_Attribute::HAlign, XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x1414d431, L"allowMacro",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::allowMacro,
     XFA_Attribute::AllowMacro, XFA_ScriptType::Basic},
    {0x1517dfa1, L"columnWidths",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::columnWidths,
     XFA_Attribute::ColumnWidths, XFA_ScriptType::Basic},
    {0x1abbd7e0, L"dataNode",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Som_DataNode,
     XFA_Attribute::Unknown, XFA_ScriptType::Object},
    {0x1ee2d24d, L"instanceIndex",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::instanceIndex,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x25839852, L"access",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,

     XFA_Attribute::Access, XFA_ScriptType::Basic},
    {0x3b1ddd06, L"fillColor",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Som_FillColor,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x570ce835, L"presence", (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::presence,
     XFA_Attribute::Presence, XFA_ScriptType::Basic},
    {0x5a3b375d, L"borderColor",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Som_BorderColor,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x7a7cc341, L"vAlign", (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::vAlign,
     XFA_Attribute::VAlign, XFA_ScriptType::Basic},
    {0x7c2ff6ae, L"maxH", (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::maxH,
     XFA_Attribute::MaxH, XFA_ScriptType::Basic},
    {0x7c2ff6bd, L"maxW", (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::maxW,
     XFA_Attribute::MaxW, XFA_ScriptType::Basic},
    {0x7d02356c, L"minH", (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::minH,
     XFA_Attribute::MinH, XFA_ScriptType::Basic},
    {0x7d02357b, L"minW", (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::minW,
     XFA_Attribute::MinW, XFA_ScriptType::Basic},
    {0x7e7e845e, L"layout", (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::layout,
     XFA_Attribute::Layout, XFA_ScriptType::Basic},
    {0x8e1c2921, L"relevant", (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::relevant,
     XFA_Attribute::Relevant, XFA_ScriptType::Basic},
    {0x9cc17d75, L"mergeMode",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,

     XFA_Attribute::MergeMode, XFA_ScriptType::Basic},
    {0x9f3e9510, L"instanceManager",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Subform_InstanceManager,
     XFA_Attribute::Unknown, XFA_ScriptType::Object},
    {0xac06e2b0, L"colSpan", (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::colSpan,
     XFA_Attribute::ColSpan, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xbc8fa350, L"locale", (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::locale,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xc2bd40fd, L"anchorType",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::anchorType,
     XFA_Attribute::AnchorType, XFA_ScriptType::Basic},
    {0xcabfa3d0, L"validationMessage",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::validationMessage,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xe4c3a5e5, L"restoreState",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::restoreState,
     XFA_Attribute::RestoreState, XFA_ScriptType::Basic},
    {0xeda9017a, L"scope", (XFA_ATTRIBUTE_CALLBACK)&CJX_Subform::scope,
     XFA_Attribute::Scope, XFA_ScriptType::Basic},
    {0xf65e34be, L"borderWidth",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Som_BorderWidth,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},

    /* select */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Select::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Select::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* window */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* localeSet */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* handler */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Handler::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x2f16a382, L"type", (XFA_ATTRIBUTE_CALLBACK)&CJX_Handler::type,
     XFA_Attribute::Type, XFA_ScriptType::Basic},
    {0x5a50e9e6, L"version", (XFA_ATTRIBUTE_CALLBACK)&CJX_Handler::version,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Handler::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* hostPseudoModel */
    {0x31b19c1, L"name", (XFA_ATTRIBUTE_CALLBACK)&CJX_HostPseudoModel::name,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x66c1ae9, L"validationsEnabled",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_HostPseudoModel::validationsEnabled,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x14d04502, L"title", (XFA_ATTRIBUTE_CALLBACK)&CJX_HostPseudoModel::title,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x392ae445, L"platform",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_HostPseudoModel::platform,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x5a50e9e6, L"version",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_HostPseudoModel::version,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x66cb1eed, L"variation",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_HostPseudoModel::variation,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x7717cbc4, L"language",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_HostPseudoModel::language,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x86698963, L"appType",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_HostPseudoModel::appType,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x94ff9e8d, L"calculationsEnabled",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_HostPseudoModel::calculationsEnabled,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xbcd44940, L"currentPage",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_HostPseudoModel::currentPage,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xd592b920, L"numPages",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_HostPseudoModel::numPages,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},

    /* presence */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* record */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* embed */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* version */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* command */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Command::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x24d85167, L"timeout", (XFA_ATTRIBUTE_CALLBACK)&CJX_Command::timeout,
     XFA_Attribute::Timeout, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Command::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* copies */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* staple */
    {0x7d9fd7c5, L"mode",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Mode, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* submitFormat */
    {0x7d9fd7c5, L"mode",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_SubmitFormat_Mode,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* boolean */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Boolean::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xa52682bd, L"{default}",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Boolean::defaultValue, XFA_Attribute::Unknown,
     XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Boolean::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xd6e27f1d, L"value", (XFA_ATTRIBUTE_CALLBACK)&CJX_Boolean::value,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},

    /* message */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Message::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Message::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* output */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* psMap */

    /* excludeNS */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* assist */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Assist::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x2038c9b2, L"role", (XFA_ATTRIBUTE_CALLBACK)&CJX_Assist::role,
     XFA_Attribute::Role, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Assist::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* picture */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Picture::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xa52682bd, L"{default}",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Picture::defaultValue, XFA_Attribute::Unknown,
     XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Picture::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xd6e27f1d, L"value", (XFA_ATTRIBUTE_CALLBACK)&CJX_Picture::value,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* traversal */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Traversal::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Traversal::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* silentPrint */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* webClient */
    {0x31b19c1, L"name",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Name, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* layoutPseudoModel */
    {0xfcef86b5, L"ready",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_LayoutPseudoModel::ready,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},

    /* producer */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* corner */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Corner::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x5392ea58, L"stroke", (XFA_ATTRIBUTE_CALLBACK)&CJX_Corner::stroke,
     XFA_Attribute::Stroke, XFA_ScriptType::Basic},
    {0x570ce835, L"presence", (XFA_ATTRIBUTE_CALLBACK)&CJX_Corner::presence,
     XFA_Attribute::Presence, XFA_ScriptType::Basic},
    {0x7b95e661, L"inverted", (XFA_ATTRIBUTE_CALLBACK)&CJX_Corner::inverted,
     XFA_Attribute::Inverted, XFA_ScriptType::Basic},
    {0x94446dcc, L"thickness", (XFA_ATTRIBUTE_CALLBACK)&CJX_Corner::thickness,
     XFA_Attribute::Thickness, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Corner::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xe8dddf50, L"join", (XFA_ATTRIBUTE_CALLBACK)&CJX_Corner::join,
     XFA_Attribute::Join, XFA_ScriptType::Basic},
    {0xe948b9a8, L"radius", (XFA_ATTRIBUTE_CALLBACK)&CJX_Corner::radius,
     XFA_Attribute::Radius, XFA_ScriptType::Basic},

    /* msgId */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* color */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Color::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xabfa6c4f, L"cSpace", (XFA_ATTRIBUTE_CALLBACK)&CJX_Color::cSpace,
     XFA_Attribute::CSpace, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Color::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xd6e27f1d, L"value", (XFA_ATTRIBUTE_CALLBACK)&CJX_Color::value,
     XFA_Attribute::Value, XFA_ScriptType::Basic},

    /* keep */
    {0x3848b3f, L"next", (XFA_ATTRIBUTE_CALLBACK)&CJX_Keep::next,
     XFA_Attribute::Next, XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Keep::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x6a3405dd, L"previous", (XFA_ATTRIBUTE_CALLBACK)&CJX_Keep::previous,
     XFA_Attribute::Previous, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Keep::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xf6b59543, L"intact", (XFA_ATTRIBUTE_CALLBACK)&CJX_Keep::intact,
     XFA_Attribute::Intact, XFA_ScriptType::Basic},

    /* query */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Query::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x268b7ec1, L"commandType",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Query::commandType,
     XFA_Attribute::CommandType, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Query::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* insert */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Insert::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Insert::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* imageEdit */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_ImageEdit::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_ImageEdit::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xbde9abda, L"data", (XFA_ATTRIBUTE_CALLBACK)&CJX_ImageEdit::data,
     XFA_Attribute::Data, XFA_ScriptType::Basic},

    /* validate */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Validate::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x5b707a35, L"scriptTest",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Validate::scriptTest,
     XFA_Attribute::ScriptTest, XFA_ScriptType::Basic},
    {0x6b6ddcfb, L"nullTest", (XFA_ATTRIBUTE_CALLBACK)&CJX_Validate::nullTest,
     XFA_Attribute::NullTest, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Validate::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xe64b1129, L"formatTest",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Validate::formatTest,
     XFA_Attribute::FormatTest, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* digestMethods */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_DigestMethods::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x2f16a382, L"type", (XFA_ATTRIBUTE_CALLBACK)&CJX_DigestMethods::type,
     XFA_Attribute::Type, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_DigestMethods::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* numberPatterns */

    /* pageSet */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_PageSet::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x8c99377e, L"relation", (XFA_ATTRIBUTE_CALLBACK)&CJX_PageSet::relation,
     XFA_Attribute::Relation, XFA_ScriptType::Basic},
    {0x8e1c2921, L"relevant", (XFA_ATTRIBUTE_CALLBACK)&CJX_PageSet::relevant,
     XFA_Attribute::Relevant, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_PageSet::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* integer */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Integer::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xa52682bd, L"{default}",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Integer::defaultValue, XFA_Attribute::Unknown,
     XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Integer::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xd6e27f1d, L"value", (XFA_ATTRIBUTE_CALLBACK)&CJX_Integer::value,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},

    /* soapAddress */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_SoapAddress::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_SoapAddress::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* equate */
    {0x25363, L"to",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::To, XFA_ScriptType::Basic},
    {0x66642f8f, L"force",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Force, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xcd7f7b54, L"from",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::From, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* formFieldFilling */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* pageRange */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* update */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Update::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Update::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* connectString */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_ConnectString::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_ConnectString::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* mode */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* layout */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* #xml */
    {0xd6e27f1d, L"value",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Value, XFA_ScriptType::Basic},

    /* xsdConnection */
    {0x2b5df51e, L"dataDescription",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_XsdConnection::dataDescription,
     XFA_Attribute::DataDescription, XFA_ScriptType::Basic},

    /* traverse */
    {0xbb8df5d, L"ref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Traverse::ref,
     XFA_Attribute::Ref, XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Traverse::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x226ca8f1, L"operation", (XFA_ATTRIBUTE_CALLBACK)&CJX_Traverse::operation,
     XFA_Attribute::Operation, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Traverse::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* encodings */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Encodings::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x2f16a382, L"type", (XFA_ATTRIBUTE_CALLBACK)&CJX_Encodings::type,
     XFA_Attribute::Type, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Encodings::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* template */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* acrobat */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* validationMessaging */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* signing */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Signing::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x2f16a382, L"type", (XFA_ATTRIBUTE_CALLBACK)&CJX_Signing::type,
     XFA_Attribute::Type, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Signing::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* dataWindow */
    {0xfb67185, L"recordsBefore",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_DataWindow::recordsBefore,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x21d5dfcb, L"currentRecordNumber",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_DataWindow::currentRecordNumber,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x312af044, L"recordsAfter",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_DataWindow::recordsAfter,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x6aab37cb, L"isDefined",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_DataWindow::isDefined, XFA_Attribute::Unknown,
     XFA_ScriptType::Basic},

    /* script */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Script::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x42fed1fd, L"contentType",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Script::contentType,
     XFA_Attribute::ContentType, XFA_ScriptType::Basic},
    {0x6cfa828a, L"runAt", (XFA_ATTRIBUTE_CALLBACK)&CJX_Script::runAt,
     XFA_Attribute::RunAt, XFA_ScriptType::Basic},
    {0xa021b738, L"stateless", (XFA_ATTRIBUTE_CALLBACK)&CJX_Script::stateless,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xa52682bd, L"{default}",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Script::defaultValue, XFA_Attribute::Unknown,
     XFA_ScriptType::Basic},
    {0xadc4c77b, L"binding", (XFA_ATTRIBUTE_CALLBACK)&CJX_Script::binding,
     XFA_Attribute::Binding, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Script::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xd6e27f1d, L"value", (XFA_ATTRIBUTE_CALLBACK)&CJX_Script::value,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* addViewerPreferences */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* alwaysEmbed */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* passwordEdit */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_PasswordEdit::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x7a0cc471, L"passwordChar",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_PasswordEdit::passwordChar,
     XFA_Attribute::PasswordChar, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_PasswordEdit::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xe6f99487, L"hScrollPolicy",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_PasswordEdit::hScrollPolicy,
     XFA_Attribute::HScrollPolicy, XFA_ScriptType::Basic},

    /* numericEdit */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_NumericEdit::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_NumericEdit::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xe6f99487, L"hScrollPolicy",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_NumericEdit::hScrollPolicy,
     XFA_Attribute::HScrollPolicy, XFA_ScriptType::Basic},

    /* encryptionMethod */
    {0xc0811ed, L"use",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* change */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* pageArea */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_PageArea::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x14a32d52, L"pagePosition",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_PageArea::pagePosition,
     XFA_Attribute::PagePosition, XFA_ScriptType::Basic},
    {0x8340ea66, L"oddOrEven", (XFA_ATTRIBUTE_CALLBACK)&CJX_PageArea::oddOrEven,
     XFA_Attribute::OddOrEven, XFA_ScriptType::Basic},
    {0x8e1c2921, L"relevant", (XFA_ATTRIBUTE_CALLBACK)&CJX_PageArea::relevant,
     XFA_Attribute::Relevant, XFA_ScriptType::Basic},
    {0xa85e74f3, L"initialNumber",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_PageArea::initialNumber,
     XFA_Attribute::InitialNumber, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_PageArea::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xbe9ba472, L"numbered", (XFA_ATTRIBUTE_CALLBACK)&CJX_PageArea::numbered,
     XFA_Attribute::Numbered, XFA_ScriptType::Basic},
    {0xd70798c2, L"blankOrNotBlank",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_PageArea::blankOrNotBlank,
     XFA_Attribute::BlankOrNotBlank, XFA_ScriptType::Basic},

    /* submitUrl */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xd6e27f1d, L"value",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Som_DefaultValue,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* oids */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Oids::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x2f16a382, L"type", (XFA_ATTRIBUTE_CALLBACK)&CJX_Oids::type,
     XFA_Attribute::Type, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Oids::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* signature */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Signature::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Signature::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* aDBE_JSConsole */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* caption */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Caption::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x34ae103c, L"reserve", (XFA_ATTRIBUTE_CALLBACK)&CJX_Caption::reserve,
     XFA_Attribute::Reserve, XFA_ScriptType::Basic},
    {0x570ce835, L"presence", (XFA_ATTRIBUTE_CALLBACK)&CJX_Caption::presence,
     XFA_Attribute::Presence, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Caption::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xf2009339, L"placement", (XFA_ATTRIBUTE_CALLBACK)&CJX_Caption::placement,
     XFA_Attribute::Placement, XFA_ScriptType::Basic},

    /* relevant */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* flipLabel */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* exData */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExData::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x42fed1fd, L"contentType",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_ExData::contentType,
     XFA_Attribute::ContentType, XFA_ScriptType::Basic},
    {0x54fa722c, L"transferEncoding",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_ExData::transferEncoding,
     XFA_Attribute::TransferEncoding, XFA_ScriptType::Basic},
    {0xa52682bd, L"{default}",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_ExData::defaultValue, XFA_Attribute::Unknown,
     XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExData::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xc4547a08, L"maxLength", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExData::maxLength,
     XFA_Attribute::MaxLength, XFA_ScriptType::Basic},
    {0xd6e27f1d, L"value",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Som_DefaultValue,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xdb55fec5, L"href", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExData::href,
     XFA_Attribute::Href, XFA_ScriptType::Basic},

    /* dayNames */
    {0x29418bb7, L"abbr",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Abbr, XFA_ScriptType::Basic},

    /* soapAction */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_SoapAction::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_SoapAction::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* defaultTypeface */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf531b059, L"writingScript",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::WritingScript, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* manifest */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Manifest::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x1b8dce3e, L"action", (XFA_ATTRIBUTE_CALLBACK)&CJX_Manifest::action,
     XFA_Attribute::Action, XFA_ScriptType::Basic},
    {0xa52682bd, L"{default}",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Manifest::defaultValue,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Manifest::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* overflow */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Overflow::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x9dcc3ab3, L"trailer", (XFA_ATTRIBUTE_CALLBACK)&CJX_Overflow::trailer,
     XFA_Attribute::Trailer, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Overflow::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xc8da4da7, L"target", (XFA_ATTRIBUTE_CALLBACK)&CJX_Overflow::target,
     XFA_Attribute::Target, XFA_ScriptType::Basic},
    {0xcbcaf66d, L"leader", (XFA_ATTRIBUTE_CALLBACK)&CJX_Overflow::leader,
     XFA_Attribute::Leader, XFA_ScriptType::Basic},

    /* linear */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Linear::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x2f16a382, L"type", (XFA_ATTRIBUTE_CALLBACK)&CJX_Linear::type,
     XFA_Attribute::Type, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Linear::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* currencySymbol */
    {0x31b19c1, L"name",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Name, XFA_ScriptType::Basic},

    /* delete */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Delete::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Delete::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* deltas */

    /* digestMethod */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_DigestMethod::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_DigestMethod::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* instanceManager */
    {0xb3543a6, L"max", (XFA_ATTRIBUTE_CALLBACK)&CJX_InstanceManager::max,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xb356ca4, L"min", (XFA_ATTRIBUTE_CALLBACK)&CJX_InstanceManager::min,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x6f544d49, L"count", (XFA_ATTRIBUTE_CALLBACK)&CJX_InstanceManager::count,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},

    /* equateRange */
    {0x25363, L"to",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::To, XFA_ScriptType::Basic},
    {0xa0933954, L"unicodeRange",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::UnicodeRange, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xcd7f7b54, L"from",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::From, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* medium */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Medium::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x4ef3d02c, L"orientation",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Medium::orientation,
     XFA_Attribute::Orientation, XFA_ScriptType::Basic},
    {0x65e30c67, L"imagingBBox",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Medium::imagingBBox,
     XFA_Attribute::ImagingBBox, XFA_ScriptType::Basic},
    {0x9041d4b0, L"short", (XFA_ATTRIBUTE_CALLBACK)&CJX_Medium::shortValue,
     XFA_Attribute::Short, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Medium::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xe349d044, L"stock", (XFA_ATTRIBUTE_CALLBACK)&CJX_Medium::stock,
     XFA_Attribute::Stock, XFA_ScriptType::Basic},
    {0xf6b4afb0, L"long", (XFA_ATTRIBUTE_CALLBACK)&CJX_Medium::longValue,
     XFA_Attribute::Long, XFA_ScriptType::Basic},

    /* textEdit */
    {0x5ce6195, L"vScrollPolicy",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_TextEdit::vScrollPolicy,
     XFA_Attribute::VScrollPolicy, XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_TextEdit::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x1ef3a64a, L"allowRichText",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_TextEdit::allowRichText,
     XFA_Attribute::AllowRichText, XFA_ScriptType::Basic},
    {0x5a32e493, L"multiLine", (XFA_ATTRIBUTE_CALLBACK)&CJX_TextEdit::multiLine,
     XFA_Attribute::MultiLine, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_TextEdit::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xe6f99487, L"hScrollPolicy",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_TextEdit::hScrollPolicy,
     XFA_Attribute::HScrollPolicy, XFA_ScriptType::Basic},

    /* templateCache */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xd52482e0, L"maxEntries",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::MaxEntries, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* compressObjectStream */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* dataValue */
    {0x42fed1fd, L"contentType",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_DataValue::contentType,
     XFA_Attribute::ContentType, XFA_ScriptType::Basic},
    {0x8855805f, L"contains", (XFA_ATTRIBUTE_CALLBACK)&CJX_DataValue::contains,
     XFA_Attribute::Contains, XFA_ScriptType::Basic},
    {0xa52682bd, L"{default}",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_DataValue::defaultValue,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xd6e27f1d, L"value", (XFA_ATTRIBUTE_CALLBACK)&CJX_DataValue::value,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xe372ae97, L"isNull", (XFA_ATTRIBUTE_CALLBACK)&CJX_DataValue::isNull,
     XFA_Attribute::IsNull, XFA_ScriptType::Basic},

    /* accessibleContent */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* nodeList */

    /* includeXDPContent */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* xmlConnection */
    {0x2b5df51e, L"dataDescription",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_XmlConnection::dataDescription,
     XFA_Attribute::DataDescription, XFA_ScriptType::Basic},

    /* validateApprovalSignatures */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* signData */
    {0xbb8df5d, L"ref", (XFA_ATTRIBUTE_CALLBACK)&CJX_SignData::ref,
     XFA_Attribute::Ref, XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_SignData::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x226ca8f1, L"operation", (XFA_ATTRIBUTE_CALLBACK)&CJX_SignData::operation,
     XFA_Attribute::Operation, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_SignData::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xc8da4da7, L"target", (XFA_ATTRIBUTE_CALLBACK)&CJX_SignData::target,
     XFA_Attribute::Target, XFA_ScriptType::Basic},

    /* packets */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* datePattern */
    {0x31b19c1, L"name",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Name, XFA_ScriptType::Basic},

    /* duplexOption */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* base */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* bind */
    {0xbb8df5d, L"ref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Bind::ref,
     XFA_Attribute::Ref, XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Bind::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x42fed1fd, L"contentType", (XFA_ATTRIBUTE_CALLBACK)&CJX_Bind::contentType,
     XFA_Attribute::ContentType, XFA_ScriptType::Basic},
    {0x54fa722c, L"transferEncoding",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Bind::transferEncoding,
     XFA_Attribute::TransferEncoding, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Bind::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xf197844d, L"match", (XFA_ATTRIBUTE_CALLBACK)&CJX_Bind::match,
     XFA_Attribute::Match, XFA_ScriptType::Basic},

    /* compression */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* user */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_User::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_User::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* rectangle */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Rectangle::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Rectangle::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xd996fa9b, L"hand", (XFA_ATTRIBUTE_CALLBACK)&CJX_Rectangle::hand,
     XFA_Attribute::Hand, XFA_ScriptType::Basic},

    /* effectiveOutputPolicy */
    {0x21aed, L"id",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Id, XFA_ScriptType::Basic},
    {0x31b19c1, L"name",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Name, XFA_ScriptType::Basic},
    {0xc0811ed, L"use",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* aDBE_JSDebugger */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* acrobat7 */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* interactive */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* locale */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* currentPage */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* data */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* date */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Date::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xa52682bd, L"{default}", (XFA_ATTRIBUTE_CALLBACK)&CJX_Date::defaultValue,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Date::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xd6e27f1d, L"value", (XFA_ATTRIBUTE_CALLBACK)&CJX_Date::value,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},

    /* desc */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Desc::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Desc::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* encrypt */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Encrypt::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x28dee6e9, L"format", (XFA_ATTRIBUTE_CALLBACK)&CJX_Encrypt::format,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Encrypt::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* draw */
    {0x68, L"h", (XFA_ATTRIBUTE_CALLBACK)&CJX_Draw::h, XFA_Attribute::H,
     XFA_ScriptType::Basic},
    {0x77, L"w", (XFA_ATTRIBUTE_CALLBACK)&CJX_Draw::w, XFA_Attribute::W,
     XFA_ScriptType::Basic},
    {0x78, L"x", (XFA_ATTRIBUTE_CALLBACK)&CJX_Draw::x, XFA_Attribute::X,
     XFA_ScriptType::Basic},
    {0x79, L"y", (XFA_ATTRIBUTE_CALLBACK)&CJX_Draw::y, XFA_Attribute::Y,
     XFA_ScriptType::Basic},
    {0x2282c73, L"hAlign", (XFA_ATTRIBUTE_CALLBACK)&CJX_Draw::hAlign,
     XFA_Attribute::HAlign, XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Draw::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x2ee7678f, L"rotate", (XFA_ATTRIBUTE_CALLBACK)&CJX_Draw::rotate,
     XFA_Attribute::Rotate, XFA_ScriptType::Basic},
    {0x570ce835, L"presence", (XFA_ATTRIBUTE_CALLBACK)&CJX_Draw::presence,
     XFA_Attribute::Presence, XFA_ScriptType::Basic},
    {0x7a7cc341, L"vAlign", (XFA_ATTRIBUTE_CALLBACK)&CJX_Draw::vAlign,
     XFA_Attribute::VAlign, XFA_ScriptType::Basic},
    {0x7c2ff6ae, L"maxH", (XFA_ATTRIBUTE_CALLBACK)&CJX_Draw::maxH,
     XFA_Attribute::MaxH, XFA_ScriptType::Basic},
    {0x7c2ff6bd, L"maxW", (XFA_ATTRIBUTE_CALLBACK)&CJX_Draw::maxW,
     XFA_Attribute::MaxW, XFA_ScriptType::Basic},
    {0x7d02356c, L"minH", (XFA_ATTRIBUTE_CALLBACK)&CJX_Draw::minH,
     XFA_Attribute::MinH, XFA_ScriptType::Basic},
    {0x7d02357b, L"minW", (XFA_ATTRIBUTE_CALLBACK)&CJX_Draw::minW,
     XFA_Attribute::MinW, XFA_ScriptType::Basic},
    {0x8e1c2921, L"relevant", (XFA_ATTRIBUTE_CALLBACK)&CJX_Draw::relevant,
     XFA_Attribute::Relevant, XFA_ScriptType::Basic},
    {0xa03cf627, L"rawValue", (XFA_ATTRIBUTE_CALLBACK)&CJX_Draw::rawValue,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xa52682bd, L"{default}", (XFA_ATTRIBUTE_CALLBACK)&CJX_Draw::defaultValue,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xac06e2b0, L"colSpan", (XFA_ATTRIBUTE_CALLBACK)&CJX_Draw::colSpan,
     XFA_Attribute::ColSpan, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Draw::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xbc8fa350, L"locale", (XFA_ATTRIBUTE_CALLBACK)&CJX_Draw::locale,
     XFA_Attribute::Locale, XFA_ScriptType::Basic},
    {0xc2bd40fd, L"anchorType", (XFA_ATTRIBUTE_CALLBACK)&CJX_Draw::anchorType,
     XFA_Attribute::AnchorType, XFA_ScriptType::Basic},

    /* encryption */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* meridiemNames */

    /* messaging */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* speak */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Speak::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x39cdb0a2, L"priority", (XFA_ATTRIBUTE_CALLBACK)&CJX_Speak::priority,
     XFA_Attribute::Priority, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Speak::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xeb511b54, L"disable", (XFA_ATTRIBUTE_CALLBACK)&CJX_Speak::disable,
     XFA_Attribute::Disable, XFA_ScriptType::Basic},

    /* dataGroup */

    /* common */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* #text */
    {0xd6e27f1d, L"value",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Value, XFA_ScriptType::Basic},

    /* paginationOverride */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* reasons */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Reasons::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x2f16a382, L"type", (XFA_ATTRIBUTE_CALLBACK)&CJX_Reasons::type,
     XFA_Attribute::Type, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Reasons::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* signatureProperties */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_SignatureProperties::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_SignatureProperties::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* threshold */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* appearanceFilter */
    {0x21aed, L"id",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Id, XFA_ScriptType::Basic},
    {0xc0811ed, L"use",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x2f16a382, L"type",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Type, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* fill */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Fill::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x570ce835, L"presence", (XFA_ATTRIBUTE_CALLBACK)&CJX_Fill::presence,
     XFA_Attribute::Presence, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Fill::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* font */
    {0xcb0ac9, L"lineThrough", (XFA_ATTRIBUTE_CALLBACK)&CJX_Font::lineThrough,
     XFA_Attribute::LineThrough, XFA_ScriptType::Basic},
    {0x2c1c7f1, L"typeface", (XFA_ATTRIBUTE_CALLBACK)&CJX_Font::typeface,
     XFA_Attribute::Typeface, XFA_ScriptType::Basic},
    {0x8c74ae9, L"fontHorizontalScale",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Font::fontHorizontalScale,
     XFA_Attribute::FontHorizontalScale, XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Font::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x2cd79033, L"kerningMode", (XFA_ATTRIBUTE_CALLBACK)&CJX_Font::kerningMode,
     XFA_Attribute::KerningMode, XFA_ScriptType::Basic},
    {0x3a0273a6, L"underline", (XFA_ATTRIBUTE_CALLBACK)&CJX_Font::underline,
     XFA_Attribute::Underline, XFA_ScriptType::Basic},
    {0x4873c601, L"baselineShift",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Font::baselineShift,
     XFA_Attribute::BaselineShift, XFA_ScriptType::Basic},
    {0x4b319767, L"overlinePeriod",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Font::overlinePeriod,
     XFA_Attribute::OverlinePeriod, XFA_ScriptType::Basic},
    {0x79543055, L"letterSpacing",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Font::letterSpacing,
     XFA_Attribute::LetterSpacing, XFA_ScriptType::Basic},
    {0x8ec6204c, L"lineThroughPeriod",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Font::lineThroughPeriod,
     XFA_Attribute::LineThroughPeriod, XFA_ScriptType::Basic},
    {0x907c7719, L"fontVerticalScale",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Font::fontVerticalScale,
     XFA_Attribute::FontVerticalScale, XFA_ScriptType::Basic},
    {0xa686975b, L"size", (XFA_ATTRIBUTE_CALLBACK)&CJX_Font::size,
     XFA_Attribute::Size, XFA_ScriptType::Basic},
    {0xb5e49bf2, L"posture", (XFA_ATTRIBUTE_CALLBACK)&CJX_Font::posture,
     XFA_Attribute::Posture, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Font::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xbd6e1d88, L"weight", (XFA_ATTRIBUTE_CALLBACK)&CJX_Font::weight,
     XFA_Attribute::Weight, XFA_ScriptType::Basic},
    {0xbd96a0e9, L"underlinePeriod",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Font::underlinePeriod,
     XFA_Attribute::UnderlinePeriod, XFA_ScriptType::Basic},
    {0xc0ec9fa4, L"overline", (XFA_ATTRIBUTE_CALLBACK)&CJX_Font::overline,
     XFA_Attribute::Overline, XFA_ScriptType::Basic},

    /* form */
    {0xaf754613, L"checksum",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Form_Checksum,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},

    /* mediumInfo */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* certificate */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Certificate::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Certificate::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* password */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Password::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Password::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* runScripts */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* trace */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* float */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Float::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xa52682bd, L"{default}", (XFA_ATTRIBUTE_CALLBACK)&CJX_Float::defaultValue,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Float::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xd6e27f1d, L"value", (XFA_ATTRIBUTE_CALLBACK)&CJX_Float::value,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},

    /* renderPolicy */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* logPseudoModel */

    /* destination */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* value */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Value::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x8e1c2921, L"relevant", (XFA_ATTRIBUTE_CALLBACK)&CJX_Value::relevant,
     XFA_Attribute::Relevant, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Value::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xea7090a0, L"override", (XFA_ATTRIBUTE_CALLBACK)&CJX_Value::override,
     XFA_Attribute::Override, XFA_ScriptType::Basic},

    /* bookend */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Bookend::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x9dcc3ab3, L"trailer", (XFA_ATTRIBUTE_CALLBACK)&CJX_Bookend::trailer,
     XFA_Attribute::Trailer, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Bookend::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xcbcaf66d, L"leader", (XFA_ATTRIBUTE_CALLBACK)&CJX_Bookend::leader,
     XFA_Attribute::Leader, XFA_ScriptType::Basic},

    /* exObject */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExObject::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x60a61edd, L"codeType", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExObject::codeType,
     XFA_Attribute::CodeType, XFA_ScriptType::Basic},
    {0xb373a862, L"archive", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExObject::archive,
     XFA_Attribute::Archive, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExObject::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xe1a26b56, L"codeBase", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExObject::codeBase,
     XFA_Attribute::CodeBase, XFA_ScriptType::Basic},
    {0xeb091003, L"classId", (XFA_ATTRIBUTE_CALLBACK)&CJX_ExObject::classId,
     XFA_Attribute::ClassId, XFA_ScriptType::Basic},

    /* openAction */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* neverEmbed */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* bindItems */
    {0x47d03490, L"connection",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_BindItems::connection,
     XFA_Attribute::Connection, XFA_ScriptType::Basic},
    {0xc39a88bd, L"labelRef", (XFA_ATTRIBUTE_CALLBACK)&CJX_BindItems::labelRef,
     XFA_Attribute::LabelRef, XFA_ScriptType::Basic},
    {0xd50f903a, L"valueRef", (XFA_ATTRIBUTE_CALLBACK)&CJX_BindItems::valueRef,
     XFA_Attribute::ValueRef, XFA_ScriptType::Basic},

    /* calculate */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Calculate::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Calculate::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xea7090a0, L"override", (XFA_ATTRIBUTE_CALLBACK)&CJX_Calculate::override,
     XFA_Attribute::Override, XFA_ScriptType::Basic},

    /* print */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* extras */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Extras::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x2f16a382, L"type", (XFA_ATTRIBUTE_CALLBACK)&CJX_Extras::type,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Extras::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* proto */

    /* dSigData */

    /* creator */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* connect */
    {0xbb8df5d, L"ref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Connect::ref,
     XFA_Attribute::Ref, XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Connect::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x24d85167, L"timeout", (XFA_ATTRIBUTE_CALLBACK)&CJX_Connect::timeout,
     XFA_Attribute::Timeout, XFA_ScriptType::Basic},
    {0x47d03490, L"connection",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Connect::connection,
     XFA_Attribute::Connection, XFA_ScriptType::Basic},
    {0x552d9ad5, L"usage", (XFA_ATTRIBUTE_CALLBACK)&CJX_Connect::usage,
     XFA_Attribute::Usage, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Connect::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xc860f30a, L"delayedOpen",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Connect::delayedOpen,
     XFA_Attribute::DelayedOpen, XFA_ScriptType::Basic},

    /* permissions */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* connectionSet */

    /* submit */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Submit::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x28dee6e9, L"format", (XFA_ATTRIBUTE_CALLBACK)&CJX_Submit::format,
     XFA_Attribute::Format, XFA_ScriptType::Basic},
    {0x824f21b7, L"embedPDF", (XFA_ATTRIBUTE_CALLBACK)&CJX_Submit::embedPDF,
     XFA_Attribute::EmbedPDF, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Submit::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xc8da4da7, L"target", (XFA_ATTRIBUTE_CALLBACK)&CJX_Submit::target,
     XFA_Attribute::Target, XFA_ScriptType::Basic},
    {0xdc75676c, L"textEncoding",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Submit::textEncoding,
     XFA_Attribute::TextEncoding, XFA_ScriptType::Basic},
    {0xf889e747, L"xdpContent", (XFA_ATTRIBUTE_CALLBACK)&CJX_Submit::xdpContent,
     XFA_Attribute::XdpContent, XFA_ScriptType::Basic},

    /* range */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* linearized */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* packet */
    {0x97be91b, L"content", (XFA_ATTRIBUTE_CALLBACK)&CJX_Packet::content,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},

    /* rootElement */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_RootElement::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_RootElement::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* plaintextMetadata */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* numberSymbols */

    /* printHighQuality */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* driver */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* incrementalLoad */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* subjectDN */
    {0x4156ee3f, L"delimiter",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_SubjectDN::delimiter,
     XFA_Attribute::Delimiter, XFA_ScriptType::Basic},

    /* compressLogicalStructure */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* incrementalMerge */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* radial */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Radial::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x2f16a382, L"type", (XFA_ATTRIBUTE_CALLBACK)&CJX_Radial::type,
     XFA_Attribute::Type, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Radial::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* variables */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Variables::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Variables::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* timePatterns */

    /* effectiveInputPolicy */
    {0x21aed, L"id",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Id, XFA_ScriptType::Basic},
    {0x31b19c1, L"name",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Name, XFA_ScriptType::Basic},
    {0xc0811ed, L"use",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* nameAttr */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* conformance */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* transform */
    {0xbb8df5d, L"ref",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Ref, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* lockDocument */
    {0x21aed, L"id",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Id, XFA_ScriptType::Basic},
    {0xc0811ed, L"use",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x2f16a382, L"type",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Type, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* breakAfter */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_BreakAfter::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x453eaf38, L"startNew", (XFA_ATTRIBUTE_CALLBACK)&CJX_BreakAfter::startNew,
     XFA_Attribute::StartNew, XFA_ScriptType::Basic},
    {0x9dcc3ab3, L"trailer", (XFA_ATTRIBUTE_CALLBACK)&CJX_BreakAfter::trailer,
     XFA_Attribute::Trailer, XFA_ScriptType::Basic},
    {0xa6118c89, L"targetType",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_BreakAfter::targetType,
     XFA_Attribute::TargetType, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_BreakAfter::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xc8da4da7, L"target", (XFA_ATTRIBUTE_CALLBACK)&CJX_BreakAfter::target,
     XFA_Attribute::Target, XFA_ScriptType::Basic},
    {0xcbcaf66d, L"leader", (XFA_ATTRIBUTE_CALLBACK)&CJX_BreakAfter::leader,
     XFA_Attribute::Leader, XFA_ScriptType::Basic},

    /* line */
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Line::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xabef37e3, L"slope", (XFA_ATTRIBUTE_CALLBACK)&CJX_Line::slope,
     XFA_Attribute::Slope, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Line::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},
    {0xd996fa9b, L"hand", (XFA_ATTRIBUTE_CALLBACK)&CJX_Line::hand,
     XFA_Attribute::Hand, XFA_ScriptType::Basic},

    /* list */
    {0xa60dd202, L"length", (XFA_ATTRIBUTE_CALLBACK)&CJX_List::length,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},

    /* source */
    {0x20146, L"db", (XFA_ATTRIBUTE_CALLBACK)&CJX_Source::db,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Source::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Source::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* occur */
    {0xb3543a6, L"max", (XFA_ATTRIBUTE_CALLBACK)&CJX_Occur::max,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xb356ca4, L"min", (XFA_ATTRIBUTE_CALLBACK)&CJX_Occur::min,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Occur::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x7d0b5fca, L"initial", (XFA_ATTRIBUTE_CALLBACK)&CJX_Occur::initial,
     XFA_Attribute::Initial, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Occur::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* pickTrayByPDFSize */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* monthNames */
    {0x29418bb7, L"abbr",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Abbr, XFA_ScriptType::Basic},

    /* severity */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* groupParent */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* documentAssembly */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* numberSymbol */
    {0x31b19c1, L"name",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Name, XFA_ScriptType::Basic},

    /* tagged */
    {0xbe52dfbf, L"desc",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_String,
     XFA_Attribute::Desc, XFA_ScriptType::Basic},
    {0xf6b47749, L"lock",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Attribute_BOOL,
     XFA_Attribute::Lock, XFA_ScriptType::Basic},

    /* items */
    {0xbb8df5d, L"ref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Items::ref,
     XFA_Attribute::Ref, XFA_ScriptType::Basic},
    {0xc0811ed, L"use", (XFA_ATTRIBUTE_CALLBACK)&CJX_Items::use,
     XFA_Attribute::Use, XFA_ScriptType::Basic},
    {0x570ce835, L"presence", (XFA_ATTRIBUTE_CALLBACK)&CJX_Items::presence,
     XFA_Attribute::Presence, XFA_ScriptType::Basic},
    {0xa5b410cf, L"save", (XFA_ATTRIBUTE_CALLBACK)&CJX_Items::save,
     XFA_Attribute::Save, XFA_ScriptType::Basic},
    {0xbc254332, L"usehref", (XFA_ATTRIBUTE_CALLBACK)&CJX_Items::usehref,
     XFA_Attribute::Usehref, XFA_ScriptType::Basic},

    /* object */
    {0xb2c80857, L"className", (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::className,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},

    /* list */
    {0xa60dd202, L"length", (XFA_ATTRIBUTE_CALLBACK)&CJX_List::length,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},

    /* [unknown] */

    /* tree */
    {0x31b19c1, L"name", (XFA_ATTRIBUTE_CALLBACK)&CJX_Tree::name,
     XFA_Attribute::Name, XFA_ScriptType::Basic},
    {0x9f9d0f9, L"all", (XFA_ATTRIBUTE_CALLBACK)&CJX_Tree::all,
     XFA_Attribute::Unknown, XFA_ScriptType::Object},
    {0x4df15659, L"nodes", (XFA_ATTRIBUTE_CALLBACK)&CJX_Tree::nodes,
     XFA_Attribute::Unknown, XFA_ScriptType::Object},
    {0x78a8d6cf, L"classAll", (XFA_ATTRIBUTE_CALLBACK)&CJX_Tree::classAll,
     XFA_Attribute::Unknown, XFA_ScriptType::Object},
    {0xcad6d8ca, L"parent", (XFA_ATTRIBUTE_CALLBACK)&CJX_Tree::parent,
     XFA_Attribute::Unknown, XFA_ScriptType::Object},
    {0xd5679c78, L"index", (XFA_ATTRIBUTE_CALLBACK)&CJX_Tree::index,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xdb5b4bce, L"classIndex", (XFA_ATTRIBUTE_CALLBACK)&CJX_Tree::classIndex,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xe4989adf, L"somExpression",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Tree::somExpression, XFA_Attribute::Unknown,
     XFA_ScriptType::Basic},

    /* node */
    {0x21aed, L"id", (XFA_ATTRIBUTE_CALLBACK)&CJX_Node::id, XFA_Attribute::Id,
     XFA_ScriptType::Basic},
    {0x234a1, L"ns", (XFA_ATTRIBUTE_CALLBACK)&CJX_Node::ns,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0x50d1a9d1, L"model", (XFA_ATTRIBUTE_CALLBACK)&CJX_Node::model,
     XFA_Attribute::Unknown, XFA_ScriptType::Object},
    {0xacb4823f, L"isContainer", (XFA_ATTRIBUTE_CALLBACK)&CJX_Node::isContainer,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xe372ae97, L"isNull", (XFA_ATTRIBUTE_CALLBACK)&CJX_Node::isNull,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xfe612a5b, L"oneOfChild", (XFA_ATTRIBUTE_CALLBACK)&CJX_Node::oneOfChild,
     XFA_Attribute::Unknown, XFA_ScriptType::Object},

    /* [unknown] */

    /* [unknown] */

    /* model */
    {0x97c1c65, L"context", (XFA_ATTRIBUTE_CALLBACK)&CJX_Model::context,
     XFA_Attribute::Unknown, XFA_ScriptType::Object},
    {0x58be2870, L"aliasNode", (XFA_ATTRIBUTE_CALLBACK)&CJX_Model::aliasNode,
     XFA_Attribute::Unknown, XFA_ScriptType::Object},

    /* [unknown] */
    {0xa52682bd, L"{default}",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Som_DefaultValue,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
    {0xd6e27f1d, L"value",
     (XFA_ATTRIBUTE_CALLBACK)&CJX_Object::Script_Som_DefaultValue,
     XFA_Attribute::Unknown, XFA_ScriptType::Basic},
};
const int32_t g_iSomAttributeCount = FX_ArraySize(g_SomAttributeData);

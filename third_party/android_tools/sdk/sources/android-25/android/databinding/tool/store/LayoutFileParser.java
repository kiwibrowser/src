/*
 * Copyright (C) 2015 The Android Open Source Project
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *      http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.databinding.tool.store;

import android.databinding.parser.XMLLexer;
import android.databinding.parser.XMLParser;
import android.databinding.parser.XMLParserBaseVisitor;
import android.databinding.tool.LayoutXmlProcessor;
import android.databinding.tool.processing.ErrorMessages;
import android.databinding.tool.processing.Scope;
import android.databinding.tool.processing.scopes.FileScopeProvider;
import android.databinding.tool.util.L;
import android.databinding.tool.util.ParserHelper;
import android.databinding.tool.util.Preconditions;
import android.databinding.tool.util.StringUtils;
import android.databinding.tool.util.XmlEditor;

import com.google.common.base.Strings;

import org.antlr.v4.runtime.ANTLRInputStream;
import org.antlr.v4.runtime.CommonTokenStream;
import org.antlr.v4.runtime.ParserRuleContext;
import org.antlr.v4.runtime.misc.NotNull;
import org.apache.commons.io.FileUtils;
import org.mozilla.universalchardet.UniversalDetector;
import org.w3c.dom.Document;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpression;
import javax.xml.xpath.XPathExpressionException;
import javax.xml.xpath.XPathFactory;

/**
 * Gets the list of XML files and creates a list of
 * {@link android.databinding.tool.store.ResourceBundle} that can be persistent or converted to
 * LayoutBinder.
 */
public class LayoutFileParser {

    private static final String XPATH_BINDING_LAYOUT = "/layout";

    private static final String LAYOUT_PREFIX = "@layout/";

    public ResourceBundle.LayoutFileBundle parseXml(final File inputFile, final File outputFile,
            String pkg, final LayoutXmlProcessor.OriginalFileLookup originalFileLookup)
            throws ParserConfigurationException, IOException, SAXException,
            XPathExpressionException {
        File originalFileFor = originalFileLookup.getOriginalFileFor(inputFile);
        final String originalFilePath = originalFileFor.getAbsolutePath();
        try {
            Scope.enter(new FileScopeProvider() {
                @Override
                public String provideScopeFilePath() {
                    return originalFilePath;
                }
            });
            final String encoding = findEncoding(inputFile);
            stripFile(inputFile, outputFile, encoding, originalFileLookup);
            return parseOriginalXml(originalFileFor, pkg, encoding);
        } finally {
            Scope.exit();
        }
    }

    private ResourceBundle.LayoutFileBundle parseOriginalXml(final File original, String pkg,
            String encoding) throws IOException {
        try {
            Scope.enter(new FileScopeProvider() {
                @Override
                public String provideScopeFilePath() {
                    return original.getAbsolutePath();
                }
            });
            final String xmlNoExtension = ParserHelper.stripExtension(original.getName());
            FileInputStream fin = new FileInputStream(original);
            InputStreamReader reader = new InputStreamReader(fin, encoding);
            ANTLRInputStream inputStream = new ANTLRInputStream(reader);
            XMLLexer lexer = new XMLLexer(inputStream);
            CommonTokenStream tokenStream = new CommonTokenStream(lexer);
            XMLParser parser = new XMLParser(tokenStream);
            XMLParser.DocumentContext expr = parser.document();
            XMLParser.ElementContext root = expr.element();
            if (!"layout".equals(root.elmName.getText())) {
                return null;
            }
            XMLParser.ElementContext data = getDataNode(root);
            XMLParser.ElementContext rootView = getViewNode(original, root);

            if (hasMergeInclude(rootView)) {
                L.e(ErrorMessages.INCLUDE_INSIDE_MERGE);
                return null;
            }
            boolean isMerge = "merge".equals(rootView.elmName.getText());

            ResourceBundle.LayoutFileBundle bundle = new ResourceBundle.LayoutFileBundle(original,
                    xmlNoExtension, original.getParentFile().getName(), pkg, isMerge);
            final String newTag = original.getParentFile().getName() + '/' + xmlNoExtension;
            parseData(original, data, bundle);
            parseExpressions(newTag, rootView, isMerge, bundle);
            return bundle;
        } finally {
            Scope.exit();
        }
    }

    private static boolean isProcessedElement(String name) {
        if (Strings.isNullOrEmpty(name)) {
            return false;
        }
        if ("view".equals(name) || "include".equals(name) || name.indexOf('.') >= 0) {
            return true;
        }
        return !name.toLowerCase().equals(name);
    }

    private void parseExpressions(String newTag, final XMLParser.ElementContext rootView,
            final boolean isMerge, ResourceBundle.LayoutFileBundle bundle) {
        final List<XMLParser.ElementContext> bindingElements
                = new ArrayList<XMLParser.ElementContext>();
        final List<XMLParser.ElementContext> otherElementsWithIds
                = new ArrayList<XMLParser.ElementContext>();
        rootView.accept(new XMLParserBaseVisitor<Void>() {
            @Override
            public Void visitElement(@NotNull XMLParser.ElementContext ctx) {
                if (filter(ctx)) {
                    bindingElements.add(ctx);
                } else {
                    String name = ctx.elmName.getText();
                    if (isProcessedElement(name) &&
                            attributeMap(ctx).containsKey("android:id")) {
                        otherElementsWithIds.add(ctx);
                    }
                }
                visitChildren(ctx);
                return null;
            }

            private boolean filter(XMLParser.ElementContext ctx) {
                if (isMerge) {
                    // account for XMLParser.ContentContext
                    if (ctx.getParent().getParent() == rootView) {
                        return true;
                    }
                } else if (ctx == rootView) {
                    return true;
                }
                return hasIncludeChild(ctx) || XmlEditor.hasExpressionAttributes(ctx) ||
                        "include".equals(ctx.elmName.getText());
            }

            private boolean hasIncludeChild(XMLParser.ElementContext ctx) {
                for (XMLParser.ElementContext child : XmlEditor.elements(ctx)) {
                    if ("include".equals(child.elmName.getText())) {
                        return true;
                    }
                }
                return false;
            }
        });

        final HashMap<XMLParser.ElementContext, String> nodeTagMap =
                new HashMap<XMLParser.ElementContext, String>();
        L.d("number of binding nodes %d", bindingElements.size());
        int tagNumber = 0;
        for (XMLParser.ElementContext parent : bindingElements) {
            final Map<String, String> attributes = attributeMap(parent);
            String nodeName = parent.elmName.getText();
            String viewName = null;
            String includedLayoutName = null;
            final String id = attributes.get("android:id");
            final String tag;
            final String originalTag = attributes.get("android:tag");
            if ("include".equals(nodeName)) {
                // get the layout attribute
                final String includeValue = attributes.get("layout");
                if (Strings.isNullOrEmpty(includeValue)) {
                    L.e("%s must include a layout", parent);
                }
                if (!includeValue.startsWith(LAYOUT_PREFIX)) {
                    L.e("included value (%s) must start with %s.",
                            includeValue, LAYOUT_PREFIX);
                }
                // if user is binding something there, there MUST be a layout file to be
                // generated.
                includedLayoutName = includeValue.substring(LAYOUT_PREFIX.length());
                final ParserRuleContext myParentContent = parent.getParent();
                Preconditions.check(myParentContent instanceof XMLParser.ContentContext,
                        "parent of an include tag must be a content context but it is %s",
                        myParentContent.getClass().getCanonicalName());
                final ParserRuleContext grandParent = myParentContent.getParent();
                Preconditions.check(grandParent instanceof XMLParser.ElementContext,
                        "grandparent of an include tag must be an element context but it is %s",
                        grandParent.getClass().getCanonicalName());
                //noinspection SuspiciousMethodCalls
                tag = nodeTagMap.get(grandParent);
            } else if ("fragment".equals(nodeName)) {
                if (XmlEditor.hasExpressionAttributes(parent)) {
                    L.e("fragments do not support data binding expressions.");
                }
                continue;
            } else {
                viewName = getViewName(parent);
                // account for XMLParser.ContentContext
                if (rootView == parent || (isMerge && parent.getParent().getParent() == rootView)) {
                    tag = newTag + "_" + tagNumber;
                } else {
                    tag = "binding_" + tagNumber;
                }
                tagNumber++;
            }
            final ResourceBundle.BindingTargetBundle bindingTargetBundle =
                    bundle.createBindingTarget(id, viewName, true, tag, originalTag,
                            new Location(parent));
            nodeTagMap.put(parent, tag);
            bindingTargetBundle.setIncludedLayout(includedLayoutName);

            for (XMLParser.AttributeContext attr : XmlEditor.expressionAttributes(parent)) {
                String value = escapeQuotes(attr.attrValue.getText(), true);
                final boolean isOneWay = value.startsWith("@{");
                final boolean isTwoWay = value.startsWith("@={");
                if (isOneWay || isTwoWay) {
                    if (value.charAt(value.length() - 1) != '}') {
                        L.e("Expecting '}' in expression '%s'", attr.attrValue.getText());
                    }
                    final int startIndex = isTwoWay ? 3 : 2;
                    final int endIndex = value.length() - 1;
                    final String strippedValue = value.substring(startIndex, endIndex);
                    Location attrLocation = new Location(attr);
                    Location valueLocation = new Location();
                    // offset to 0 based
                    valueLocation.startLine = attr.attrValue.getLine() - 1;
                    valueLocation.startOffset = attr.attrValue.getCharPositionInLine() +
                            attr.attrValue.getText().indexOf(strippedValue);
                    valueLocation.endLine = attrLocation.endLine;
                    valueLocation.endOffset = attrLocation.endOffset - 2; // account for: "}
                    bindingTargetBundle.addBinding(escapeQuotes(attr.attrName.getText(), false),
                            strippedValue, isTwoWay, attrLocation, valueLocation);
                }
            }
        }

        for (XMLParser.ElementContext elm : otherElementsWithIds) {
            final String id = attributeMap(elm).get("android:id");
            final String className = getViewName(elm);
            bundle.createBindingTarget(id, className, true, null, null, new Location(elm));
        }
    }

    private String getViewName(XMLParser.ElementContext elm) {
        String viewName = elm.elmName.getText();
        if ("view".equals(viewName)) {
            String classNode = attributeMap(elm).get("class");
            if (Strings.isNullOrEmpty(classNode)) {
                L.e("No class attribute for 'view' node");
            }
            viewName = classNode;
        } else if ("include".equals(viewName) && !XmlEditor.hasExpressionAttributes(elm)) {
            viewName = "android.view.View";
        }
        return viewName;
    }

    private void parseData(File xml, XMLParser.ElementContext data,
            ResourceBundle.LayoutFileBundle bundle) {
        if (data == null) {
            return;
        }
        for (XMLParser.ElementContext imp : filter(data, "import")) {
            final Map<String, String> attrMap = attributeMap(imp);
            String type = attrMap.get("type");
            String alias = attrMap.get("alias");
            Preconditions.check(StringUtils.isNotBlank(type), "Type of an import cannot be empty."
                    + " %s in %s", imp.toStringTree(), xml);
            if (Strings.isNullOrEmpty(alias)) {
                alias = type.substring(type.lastIndexOf('.') + 1);
            }
            bundle.addImport(alias, type, new Location(imp));
        }

        for (XMLParser.ElementContext variable : filter(data, "variable")) {
            final Map<String, String> attrMap = attributeMap(variable);
            String type = attrMap.get("type");
            String name = attrMap.get("name");
            Preconditions.checkNotNull(type, "variable must have a type definition %s in %s",
                    variable.toStringTree(), xml);
            Preconditions.checkNotNull(name, "variable must have a name %s in %s",
                    variable.toStringTree(), xml);
            bundle.addVariable(name, type, new Location(variable), true);
        }
        final XMLParser.AttributeContext className = findAttribute(data, "class");
        if (className != null) {
            final String name = escapeQuotes(className.attrValue.getText(), true);
            if (StringUtils.isNotBlank(name)) {
                Location location = new Location(
                        className.attrValue.getLine() - 1,
                        className.attrValue.getCharPositionInLine() + 1,
                        className.attrValue.getLine() - 1,
                        className.attrValue.getCharPositionInLine() + name.length()
                );
                bundle.setBindingClass(name, location);
            }
        }
    }

    private XMLParser.ElementContext getDataNode(XMLParser.ElementContext root) {
        final List<XMLParser.ElementContext> data = filter(root, "data");
        if (data.size() == 0) {
            return null;
        }
        Preconditions.check(data.size() == 1, "XML layout can have only 1 data tag");
        return data.get(0);
    }

    private XMLParser.ElementContext getViewNode(File xml, XMLParser.ElementContext root) {
        final List<XMLParser.ElementContext> view = filterNot(root, "data");
        Preconditions.check(view.size() == 1, "XML layout %s must have 1 view but has %s. root"
                        + " children count %s", xml, view.size(), root.getChildCount());
        return view.get(0);
    }

    private List<XMLParser.ElementContext> filter(XMLParser.ElementContext root,
            String name) {
        List<XMLParser.ElementContext> result = new ArrayList<XMLParser.ElementContext>();
        if (root == null) {
            return result;
        }
        final XMLParser.ContentContext content = root.content();
        if (content == null) {
            return result;
        }
        for (XMLParser.ElementContext child : XmlEditor.elements(root)) {
            if (name.equals(child.elmName.getText())) {
                result.add(child);
            }
        }
        return result;
    }

    private List<XMLParser.ElementContext> filterNot(XMLParser.ElementContext root,
            String name) {
        List<XMLParser.ElementContext> result = new ArrayList<XMLParser.ElementContext>();
        if (root == null) {
            return result;
        }
        final XMLParser.ContentContext content = root.content();
        if (content == null) {
            return result;
        }
        for (XMLParser.ElementContext child : XmlEditor.elements(root)) {
            if (!name.equals(child.elmName.getText())) {
                result.add(child);
            }
        }
        return result;
    }

    private boolean hasMergeInclude(XMLParser.ElementContext rootView) {
        return "merge".equals(rootView.elmName.getText()) && filter(rootView, "include").size() > 0;
    }

    private void stripFile(File xml, File out, String encoding,
            LayoutXmlProcessor.OriginalFileLookup originalFileLookup)
            throws ParserConfigurationException, IOException, SAXException,
            XPathExpressionException {
        DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
        DocumentBuilder builder = factory.newDocumentBuilder();
        Document doc = builder.parse(xml);
        XPathFactory xPathFactory = XPathFactory.newInstance();
        XPath xPath = xPathFactory.newXPath();
        File actualFile = originalFileLookup == null ? null
                : originalFileLookup.getOriginalFileFor(xml);
        // TODO get rid of original file lookup
        if (actualFile == null) {
            actualFile = xml;
        }
        // always create id from actual file when available. Gradle may duplicate files.
        String noExt = ParserHelper.stripExtension(actualFile.getName());
        String binderId = actualFile.getParentFile().getName() + '/' + noExt;
        // now if file has any binding expressions, find and delete them
        boolean changed = isBindingLayout(doc, xPath);
        if (changed) {
            stripBindingTags(xml, out, binderId, encoding);
        } else if (!xml.equals(out)){
            FileUtils.copyFile(xml, out);
        }
    }

    private boolean isBindingLayout(Document doc, XPath xPath) throws XPathExpressionException {
        return !get(doc, xPath, XPATH_BINDING_LAYOUT).isEmpty();
    }

    private List<Node> get(Document doc, XPath xPath, String pattern)
            throws XPathExpressionException {
        final XPathExpression expr = xPath.compile(pattern);
        return toList((NodeList) expr.evaluate(doc, XPathConstants.NODESET));
    }

    private List<Node> toList(NodeList nodeList) {
        List<Node> result = new ArrayList<Node>();
        for (int i = 0; i < nodeList.getLength(); i++) {
            result.add(nodeList.item(i));
        }
        return result;
    }

    private void stripBindingTags(File xml, File output, String newTag, String encoding) throws IOException {
        String res = XmlEditor.strip(xml, newTag, encoding);
        Preconditions.checkNotNull(res, "layout file should've changed %s", xml.getAbsolutePath());
        if (res != null) {
            L.d("file %s has changed, overwriting %s", xml.getName(), xml.getAbsolutePath());
            FileUtils.writeStringToFile(output, res, encoding);
        }
    }

    private static String findEncoding(File f) throws IOException {
        FileInputStream fin = new FileInputStream(f);
        try {
            UniversalDetector universalDetector = new UniversalDetector(null);

            byte[] buf = new byte[4096];
            int nread;
            while ((nread = fin.read(buf)) > 0 && !universalDetector.isDone()) {
                universalDetector.handleData(buf, 0, nread);
            }

            universalDetector.dataEnd();

            String encoding = universalDetector.getDetectedCharset();
            if (encoding == null) {
                encoding = "utf-8";
            }
            return encoding;
        } finally {
            fin.close();
        }
    }

    private static Map<String, String> attributeMap(XMLParser.ElementContext root) {
        final Map<String, String> result = new HashMap<String, String>();
        for (XMLParser.AttributeContext attr : XmlEditor.attributes(root)) {
            result.put(escapeQuotes(attr.attrName.getText(), false),
                    escapeQuotes(attr.attrValue.getText(), true));
        }
        return result;
    }

    private static XMLParser.AttributeContext findAttribute(XMLParser.ElementContext element,
            String name) {
        for (XMLParser.AttributeContext attr : element.attribute()) {
            if (escapeQuotes(attr.attrName.getText(), false).equals(name)) {
                return attr;
            }
        }
        return null;
    }

    private static String escapeQuotes(String textWithQuotes, boolean unescapeValue) {
        char first = textWithQuotes.charAt(0);
        int start = 0, end = textWithQuotes.length();
        if (first == '"' || first == '\'') {
            start = 1;
        }
        char last = textWithQuotes.charAt(textWithQuotes.length() - 1);
        if (last == '"' || last == '\'') {
            end -= 1;
        }
        String val = textWithQuotes.substring(start, end);
        if (unescapeValue) {
            return StringUtils.unescapeXml(val);
        } else {
            return val;
        }
    }
}

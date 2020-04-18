/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.databinding.tool.util;

import android.databinding.parser.BindingExpressionLexer;
import android.databinding.parser.BindingExpressionParser;
import android.databinding.parser.XMLLexer;
import android.databinding.parser.XMLParser;
import android.databinding.parser.XMLParser.AttributeContext;
import android.databinding.parser.XMLParser.ElementContext;

import com.google.common.base.Joiner;
import com.google.common.xml.XmlEscapers;

import org.antlr.v4.runtime.ANTLRInputStream;
import org.antlr.v4.runtime.CommonTokenStream;
import org.antlr.v4.runtime.Token;
import org.antlr.v4.runtime.tree.TerminalNode;
import org.apache.commons.io.FileUtils;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

/**
 * Ugly inefficient class to strip unwanted tags from XML.
 * Band-aid solution to unblock development
 */
public class XmlEditor {

    public static String strip(File f, String newTag, String encoding) throws IOException {
        FileInputStream fin = new FileInputStream(f);
        InputStreamReader reader = new InputStreamReader(fin, encoding);
        ANTLRInputStream inputStream = new ANTLRInputStream(reader);
        XMLLexer lexer = new XMLLexer(inputStream);
        CommonTokenStream tokenStream = new CommonTokenStream(lexer);
        XMLParser parser = new XMLParser(tokenStream);
        XMLParser.DocumentContext expr = parser.document();
        ElementContext root = expr.element();

        if (root == null || !"layout".equals(nodeName(root))) {
            return null; // not a binding layout
        }

        List<? extends ElementContext> childrenOfRoot = elements(root);
        List<? extends ElementContext> dataNodes = filterNodesByName("data", childrenOfRoot);
        if (dataNodes.size() > 1) {
            L.e("Multiple binding data tags in %s. Expecting a maximum of one.",
                    f.getAbsolutePath());
        }

        ArrayList<String> lines = new ArrayList<String>();
        lines.addAll(FileUtils.readLines(f, "utf-8"));

        for (ElementContext it : dataNodes) {
            replace(lines, toPosition(it.getStart()), toEndPosition(it.getStop()), "");
        }
        List<? extends ElementContext> layoutNodes =
                excludeNodesByName("data", childrenOfRoot);
        if (layoutNodes.size() != 1) {
            L.e("Only one layout element and one data element are allowed. %s has %d",
                    f.getAbsolutePath(), layoutNodes.size());
        }

        final ElementContext layoutNode = layoutNodes.get(0);

        ArrayList<TagAndContext> noTag = new ArrayList<TagAndContext>();

        recurseReplace(layoutNode, lines, noTag, newTag, 0);

        // Remove the <layout>
        Position rootStartTag = toPosition(root.getStart());
        Position rootEndTag = toPosition(root.content().getStart());
        replace(lines, rootStartTag, rootEndTag, "");

        // Remove the </layout>
        PositionPair endLayoutPositions = findTerminalPositions(root, lines);
        replace(lines, endLayoutPositions.left, endLayoutPositions.right, "");

        StringBuilder rootAttributes = new StringBuilder();
        for (AttributeContext attr : attributes(root)) {
            rootAttributes.append(' ').append(attr.getText());
        }
        TagAndContext noTagRoot = null;
        for (TagAndContext tagAndContext : noTag) {
            if (tagAndContext.getContext() == layoutNode) {
                noTagRoot = tagAndContext;
                break;
            }
        }
        if (noTagRoot != null) {
            TagAndContext newRootTag = new TagAndContext(
                    noTagRoot.getTag() + rootAttributes.toString(), layoutNode);
            int index = noTag.indexOf(noTagRoot);
            noTag.set(index, newRootTag);
        } else {
            TagAndContext newRootTag =
                    new TagAndContext(rootAttributes.toString(), layoutNode);
            noTag.add(newRootTag);
        }
        //noinspection NullableProblems
        Collections.sort(noTag, new Comparator<TagAndContext>() {
            @Override
            public int compare(TagAndContext o1, TagAndContext o2) {
                Position start1 = toPosition(o1.getContext().getStart());
                Position start2 = toPosition(o2.getContext().getStart());
                int lineCmp = start2.line - start1.line;
                if (lineCmp != 0) {
                    return lineCmp;
                }
                return start2.charIndex - start1.charIndex;
            }
        });
        for (TagAndContext it : noTag) {
            ElementContext element = it.getContext();
            String tag = it.getTag();
            Position endTagPosition = endTagPosition(element);
            fixPosition(lines, endTagPosition);
            String line = lines.get(endTagPosition.line);
            String newLine = line.substring(0, endTagPosition.charIndex) + " " + tag +
                    line.substring(endTagPosition.charIndex);
            lines.set(endTagPosition.line, newLine);
        }
        return Joiner.on(StringUtils.LINE_SEPARATOR).join(lines);
    }

    private static <T extends XMLParser.ElementContext> List<T>
            filterNodesByName(String name, Iterable<T> items) {
        List<T> result = new ArrayList<T>();
        for (T item : items) {
            if (name.equals(nodeName(item))) {
                result.add(item);
            }
        }
        return result;
    }

    private static <T extends XMLParser.ElementContext> List<T>
            excludeNodesByName(String name, Iterable<T> items) {
        List<T> result = new ArrayList<T>();
        for (T item : items) {
            if (!name.equals(nodeName(item))) {
                result.add(item);
            }
        }
        return result;
    }

    private static Position toPosition(Token token) {
        return new Position(token.getLine() - 1, token.getCharPositionInLine());
    }

    private static Position toEndPosition(Token token) {
        return new Position(token.getLine() - 1,
                token.getCharPositionInLine() + token.getText().length());
    }

    public static String nodeName(ElementContext elementContext) {
        return elementContext.elmName.getText();
    }

    public static List<? extends AttributeContext> attributes(ElementContext elementContext) {
        if (elementContext.attribute() == null)
            return new ArrayList<AttributeContext>();
        else {
            return elementContext.attribute();
        }
    }

    public static List<? extends AttributeContext> expressionAttributes(
            ElementContext elementContext) {
        List<AttributeContext> result = new ArrayList<AttributeContext>();
        for (AttributeContext input : attributes(elementContext)) {
            String attrName = input.attrName.getText();
            boolean isExpression = attrName.equals("android:tag");
            if (!isExpression) {
                final String value = input.attrValue.getText();
                isExpression = isExpressionText(input.attrValue.getText());
            }
            if (isExpression) {
                result.add(input);
            }
        }
        return result;
    }

    private static boolean isExpressionText(String value) {
        // Check if the expression ends with "}" and starts with "@{" or "@={", ignoring
        // the surrounding quotes.
        return (value.length() > 5 && value.charAt(value.length() - 2) == '}' &&
                ("@{".equals(value.substring(1, 3)) || "@={".equals(value.substring(1, 4))));
    }

    private static Position endTagPosition(ElementContext context) {
        if (context.content() == null) {
            // no content, so just choose the start of the "/>"
            Position endTag = toPosition(context.getStop());
            if (endTag.charIndex <= 0) {
                L.e("invalid input in %s", context);
            }
            return endTag;
        } else {
            // tag with no attributes, but with content
            Position position = toPosition(context.content().getStart());
            if (position.charIndex <= 0) {
                L.e("invalid input in %s", context);
            }
            position.charIndex--;
            return position;
        }
    }

    public static List<? extends ElementContext> elements(ElementContext context) {
        if (context.content() != null && context.content().element() != null) {
            return context.content().element();
        }
        return new ArrayList<ElementContext>();
    }

    private static boolean replace(ArrayList<String> lines, Position start, Position end,
            String text) {
        fixPosition(lines, start);
        fixPosition(lines, end);
        if (start.line != end.line) {
            String startLine = lines.get(start.line);
            String newStartLine = startLine.substring(0, start.charIndex) + text;
            lines.set(start.line, newStartLine);
            for (int i = start.line + 1; i < end.line; i++) {
                String line = lines.get(i);
                lines.set(i, replaceWithSpaces(line, 0, line.length() - 1));
            }
            String endLine = lines.get(end.line);
            String newEndLine = replaceWithSpaces(endLine, 0, end.charIndex - 1);
            lines.set(end.line, newEndLine);
            return true;
        } else if (end.charIndex - start.charIndex >= text.length()) {
            String line = lines.get(start.line);
            int endTextIndex = start.charIndex + text.length();
            String replacedText = replaceRange(line, start.charIndex, endTextIndex, text);
            String spacedText = replaceWithSpaces(replacedText, endTextIndex, end.charIndex - 1);
            lines.set(start.line, spacedText);
            return true;
        } else {
            String line = lines.get(start.line);
            String newLine = replaceWithSpaces(line, start.charIndex, end.charIndex - 1);
            lines.set(start.line, newLine);
            return false;
        }
    }

    private static String replaceRange(String line, int start, int end, String newText) {
        return line.substring(0, start) + newText + line.substring(end);
    }

    public static boolean hasExpressionAttributes(ElementContext context) {
        List<? extends AttributeContext> expressions = expressionAttributes(context);
        int size = expressions.size();
        if (size == 0) {
            return false;
        } else if (size > 1) {
            return true;
        } else {
            // android:tag is included, regardless, so we must only count as an expression
            // if android:tag has a binding expression.
            return isExpressionText(expressions.get(0).attrValue.getText());
        }
    }

    private static int recurseReplace(ElementContext node, ArrayList<String> lines,
            ArrayList<TagAndContext> noTag,
            String newTag, int bindingIndex) {
        int nextBindingIndex = bindingIndex;
        boolean isMerge = "merge".equals(nodeName(node));
        final boolean containsInclude = filterNodesByName("include", elements(node)).size() > 0;
        if (!isMerge && (hasExpressionAttributes(node) || newTag != null || containsInclude)) {
            String tag = "";
            if (newTag != null) {
                tag = "android:tag=\"" + newTag + "_" + bindingIndex + "\"";
                nextBindingIndex++;
            } else if (!"include".equals(nodeName(node))) {
                tag = "android:tag=\"binding_" + bindingIndex + "\"";
                nextBindingIndex++;
            }
            for (AttributeContext it : expressionAttributes(node)) {
                Position start = toPosition(it.getStart());
                Position end = toEndPosition(it.getStop());
                String defaultVal = defaultReplacement(it);
                if (defaultVal != null) {
                    replace(lines, start, end, it.attrName.getText() + "=\"" + defaultVal + "\"");
                } else if (replace(lines, start, end, tag)) {
                    tag = "";
                }
            }
            if (tag.length() != 0) {
                noTag.add(new TagAndContext(tag, node));
            }
        }

        String nextTag;
        if (bindingIndex == 0 && isMerge) {
            nextTag = newTag;
        } else {
            nextTag = null;
        }
        for (ElementContext it : elements(node)) {
            nextBindingIndex = recurseReplace(it, lines, noTag, nextTag, nextBindingIndex);
        }
        return nextBindingIndex;
    }

    private static String defaultReplacement(XMLParser.AttributeContext attr) {
        String textWithQuotes = attr.attrValue.getText();
        String escapedText = textWithQuotes.substring(1, textWithQuotes.length() - 1);
        final boolean isTwoWay = escapedText.startsWith("@={");
        final boolean isOneWay = escapedText.startsWith("@{");
        if ((!isTwoWay && !isOneWay) || !escapedText.endsWith("}")) {
            return null;
        }
        final int startIndex = isTwoWay ? 3 : 2;
        final int endIndex = escapedText.length() - 1;
        String text = StringUtils.unescapeXml(escapedText.substring(startIndex, endIndex));
        ANTLRInputStream inputStream = new ANTLRInputStream(text);
        BindingExpressionLexer lexer = new BindingExpressionLexer(inputStream);
        CommonTokenStream tokenStream = new CommonTokenStream(lexer);
        BindingExpressionParser parser = new BindingExpressionParser(tokenStream);
        BindingExpressionParser.BindingSyntaxContext root = parser.bindingSyntax();
        BindingExpressionParser.DefaultsContext defaults = root.defaults();
        if (defaults != null) {
            BindingExpressionParser.ConstantValueContext constantValue = defaults
                    .constantValue();
            BindingExpressionParser.LiteralContext literal = constantValue.literal();
            if (literal != null) {
                BindingExpressionParser.StringLiteralContext stringLiteral = literal
                        .stringLiteral();
                if (stringLiteral != null) {
                    TerminalNode doubleQuote = stringLiteral.DoubleQuoteString();
                    if (doubleQuote != null) {
                        String quotedStr = doubleQuote.getText();
                        String unquoted = quotedStr.substring(1, quotedStr.length() - 1);
                        return XmlEscapers.xmlAttributeEscaper().escape(unquoted);
                    } else {
                        String quotedStr = stringLiteral.SingleQuoteString().getText();
                        String unquoted = quotedStr.substring(1, quotedStr.length() - 1);
                        String unescaped = unquoted.replace("\"", "\\\"").replace("\\`", "`");
                        return XmlEscapers.xmlAttributeEscaper().escape(unescaped);
                    }
                }
            }
            return constantValue.getText();
        }
        return null;
    }

    private static PositionPair findTerminalPositions(ElementContext node,
            ArrayList<String> lines) {
        Position endPosition = toEndPosition(node.getStop());
        Position startPosition = toPosition(node.getStop());
        int index;
        do {
            index = lines.get(startPosition.line).lastIndexOf("</");
            startPosition.line--;
        } while (index < 0);
        startPosition.line++;
        startPosition.charIndex = index;
        //noinspection unchecked
        return new PositionPair(startPosition, endPosition);
    }

    private static String replaceWithSpaces(String line, int start, int end) {
        StringBuilder lineBuilder = new StringBuilder(line);
        for (int i = start; i <= end; i++) {
            lineBuilder.setCharAt(i, ' ');
        }
        return lineBuilder.toString();
    }

    private static void fixPosition(ArrayList<String> lines, Position pos) {
        String line = lines.get(pos.line);
        while (pos.charIndex > line.length()) {
            pos.charIndex--;
        }
    }

    private static class Position {

        int line;
        int charIndex;

        public Position(int line, int charIndex) {
            this.line = line;
            this.charIndex = charIndex;
        }
    }

    private static class TagAndContext {
        private final String mTag;
        private final ElementContext mElementContext;

        private TagAndContext(String tag, ElementContext elementContext) {
            mTag = tag;
            mElementContext = elementContext;
        }

        private ElementContext getContext() {
            return mElementContext;
        }

        private String getTag() {
            return mTag;
        }
    }

    private static class PositionPair {
        private final Position left;
        private final Position right;

        private PositionPair(Position left, Position right) {
            this.left = left;
            this.right = right;
        }
    }
}

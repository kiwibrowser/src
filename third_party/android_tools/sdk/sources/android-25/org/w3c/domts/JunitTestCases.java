/*
 * Copyright (C) 2009 The Android Open Source Project
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
package org.w3c.domts;

import junit.framework.TestCase;

/**
 * Wrapper class to make the W3C DOM test work as JUnit test cases. Note this
 * file has been generated, so the method names may be cryptic and also be
 * longer than 80 characters per line.
 */
public class JunitTestCases extends TestCase {

    private void runDomTest(String name) throws Throwable {
        JUnitTestCaseAdapter adapter = new JUnitTestCaseAdapter();
        adapter.setName(name);
        adapter.runTest();
    }
    
    public void test_level1_core_attrcreatedocumentfragment() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/attrcreatedocumentfragment");
    }

    public void test_level1_core_attrcreatetextnode() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/attrcreatetextnode");
    }

    public void test_level1_core_attrcreatetextnode2() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/attrcreatetextnode2");
    }

    public void test_level1_core_attreffectivevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/attreffectivevalue");
    }

    public void test_level1_core_attrname() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/attrname");
    }

    public void test_level1_core_attrnextsiblingnull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/attrnextsiblingnull");
    }

    public void test_level1_core_attrparentnodenull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/attrparentnodenull");
    }

    public void test_level1_core_attrprevioussiblingnull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/attrprevioussiblingnull");
    }

    public void test_level1_core_attrspecifiedvalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/attrspecifiedvalue");
    }

    public void test_level1_core_attrspecifiedvaluechanged() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/attrspecifiedvaluechanged");
    }

    public void test_level1_core_cdatasectiongetdata() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/cdatasectiongetdata");
    }

    public void test_level1_core_characterdataappenddata() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/characterdataappenddata");
    }

    public void test_level1_core_characterdataappenddatagetdata() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/characterdataappenddatagetdata");
    }

    public void test_level1_core_characterdatadeletedatabegining() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/characterdatadeletedatabegining");
    }

    public void test_level1_core_characterdatadeletedataend() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/characterdatadeletedataend");
    }

    public void test_level1_core_characterdatadeletedataexceedslength() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/characterdatadeletedataexceedslength");
    }

    public void test_level1_core_characterdatadeletedatagetlengthanddata() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/characterdatadeletedatagetlengthanddata");
    }

    public void test_level1_core_characterdatadeletedatamiddle() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/characterdatadeletedatamiddle");
    }

    public void test_level1_core_characterdatagetdata() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/characterdatagetdata");
    }

    public void test_level1_core_characterdatagetlength() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/characterdatagetlength");
    }

    public void test_level1_core_characterdatainsertdatabeginning() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/characterdatainsertdatabeginning");
    }

    public void test_level1_core_characterdatainsertdataend() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/characterdatainsertdataend");
    }

    public void test_level1_core_characterdatainsertdatamiddle() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/characterdatainsertdatamiddle");
    }

    public void test_level1_core_characterdatareplacedatabegining() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/characterdatareplacedatabegining");
    }

    public void test_level1_core_characterdatareplacedataend() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/characterdatareplacedataend");
    }

    public void test_level1_core_characterdatareplacedataexceedslengthofarg() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/characterdatareplacedataexceedslengthofarg");
    }

    public void test_level1_core_characterdatareplacedataexceedslengthofdata() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/characterdatareplacedataexceedslengthofdata");
    }

    public void test_level1_core_characterdatareplacedatamiddle() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/characterdatareplacedatamiddle");
    }

    public void test_level1_core_characterdatasubstringvalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/characterdatasubstringvalue");
    }

    public void test_level1_core_commentgetcomment() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/commentgetcomment");
    }

    public void test_level1_core_documentcreatecdatasection() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/documentcreatecdatasection");
    }

    public void test_level1_core_documentcreatecomment() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/documentcreatecomment");
    }

    public void test_level1_core_documentcreatedocumentfragment() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/documentcreatedocumentfragment");
    }

    public void test_level1_core_documentcreateelement() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/documentcreateelement");
    }

    public void test_level1_core_documentcreateelementcasesensitive() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/documentcreateelementcasesensitive");
    }

    public void test_level1_core_documentcreateentityreference() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/documentcreateentityreference");
    }

    public void test_level1_core_documentcreateprocessinginstruction() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/documentcreateprocessinginstruction");
    }

    public void test_level1_core_documentcreatetextnode() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/documentcreatetextnode");
    }

    public void test_level1_core_documentgetdoctype() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/documentgetdoctype");
    }

    public void test_level1_core_documentgetdoctypenodtd() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/documentgetdoctypenodtd");
    }

    public void test_level1_core_documentgetelementsbytagnamelength() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/documentgetelementsbytagnamelength");
    }

    public void test_level1_core_documentgetelementsbytagnamevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/documentgetelementsbytagnamevalue");
    }

    public void test_level1_core_documentgetimplementation() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/documentgetimplementation");
    }

    public void test_level1_core_documentgetrootnode() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/documentgetrootnode");
    }

    public void test_level1_core_documentinvalidcharacterexceptioncreateattribute() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/documentinvalidcharacterexceptioncreateattribute");
    }

    public void test_level1_core_documentinvalidcharacterexceptioncreateelement() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/documentinvalidcharacterexceptioncreateelement");
    }

    public void test_level1_core_documenttypegetdoctype() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/documenttypegetdoctype");
    }

    public void test_level1_core_domimplementationfeaturenoversion() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/domimplementationfeaturenoversion");
    }

    public void test_level1_core_domimplementationfeaturenull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/domimplementationfeaturenull");
    }

    public void test_level1_core_domimplementationfeaturexml() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/domimplementationfeaturexml");
    }

    public void test_level1_core_elementaddnewattribute() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/elementaddnewattribute");
    }

    public void test_level1_core_elementassociatedattribute() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/elementassociatedattribute");
    }

    public void test_level1_core_elementchangeattributevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/elementchangeattributevalue");
    }

    public void test_level1_core_elementgetattributenode() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/elementgetattributenode");
    }

    public void test_level1_core_elementgetattributenodenull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/elementgetattributenodenull");
    }

    public void test_level1_core_elementgetelementsbytagname() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/elementgetelementsbytagname");
    }

    public void test_level1_core_elementgetelementsbytagnameaccessnodelist() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/elementgetelementsbytagnameaccessnodelist");
    }

    public void test_level1_core_elementgetelementsbytagnamenomatch() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/elementgetelementsbytagnamenomatch");
    }

    public void test_level1_core_elementgettagname() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/elementgettagname");
    }

    public void test_level1_core_elementinuseattributeerr() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/elementinuseattributeerr");
    }

    public void test_level1_core_elementinvalidcharacterexception() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/elementinvalidcharacterexception");
    }

    public void test_level1_core_elementnormalize() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/elementnormalize");
    }

    public void test_level1_core_elementnotfounderr() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/elementnotfounderr");
    }

    public void test_level1_core_elementremoveattributeaftercreate() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/elementremoveattributeaftercreate");
    }

    public void test_level1_core_elementremoveattributenode() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/elementremoveattributenode");
    }

    public void test_level1_core_elementreplaceexistingattributegevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/elementreplaceexistingattributegevalue");
    }

    public void test_level1_core_elementretrieveattrvalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/elementretrieveattrvalue");
    }

    public void test_level1_core_elementretrievetagname() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/elementretrievetagname");
    }

    public void test_level1_core_elementsetattributenodenull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/elementsetattributenodenull");
    }

    public void test_level1_core_elementwrongdocumenterr() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/elementwrongdocumenterr");
    }

    public void test_level1_core_namednodemapchildnoderange() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/namednodemapchildnoderange");
    }

    public void test_level1_core_namednodemapgetnameditem() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/namednodemapgetnameditem");
    }

    public void test_level1_core_namednodemapinuseattributeerr() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/namednodemapinuseattributeerr");
    }

    public void test_level1_core_namednodemapnotfounderr() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/namednodemapnotfounderr");
    }

    public void test_level1_core_namednodemapnumberofnodes() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/namednodemapnumberofnodes");
    }

    public void test_level1_core_namednodemapremovenameditemreturnnodevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/namednodemapremovenameditemreturnnodevalue");
    }

    public void test_level1_core_namednodemapreturnattrnode() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/namednodemapreturnattrnode");
    }

    public void test_level1_core_namednodemapreturnfirstitem() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/namednodemapreturnfirstitem");
    }

    public void test_level1_core_namednodemapreturnlastitem() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/namednodemapreturnlastitem");
    }

    public void test_level1_core_namednodemapreturnnull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/namednodemapreturnnull");
    }

    public void test_level1_core_namednodemapsetnameditem() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/namednodemapsetnameditem");
    }

    public void test_level1_core_namednodemapsetnameditemreturnvalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/namednodemapsetnameditemreturnvalue");
    }

    public void test_level1_core_namednodemapsetnameditemwithnewvalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/namednodemapsetnameditemwithnewvalue");
    }

    public void test_level1_core_namednodemapwrongdocumenterr() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/namednodemapwrongdocumenterr");
    }

    public void test_level1_core_nodeappendchild() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeappendchild");
    }

    public void test_level1_core_nodeappendchildgetnodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeappendchildgetnodename");
    }

    public void test_level1_core_nodeappendchildnewchilddiffdocument() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeappendchildnewchilddiffdocument");
    }

    public void test_level1_core_nodeappendchildnodeancestor() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeappendchildnodeancestor");
    }

    public void test_level1_core_nodeattributenodeattribute() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeattributenodeattribute");
    }

    public void test_level1_core_nodeattributenodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeattributenodename");
    }

    public void test_level1_core_nodeattributenodetype() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeattributenodetype");
    }

    public void test_level1_core_nodeattributenodevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeattributenodevalue");
    }

    public void test_level1_core_nodecdatasectionnodeattribute() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodecdatasectionnodeattribute");
    }

    public void test_level1_core_nodecdatasectionnodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodecdatasectionnodename");
    }

    public void test_level1_core_nodecdatasectionnodetype() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodecdatasectionnodetype");
    }

    public void test_level1_core_nodecdatasectionnodevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodecdatasectionnodevalue");
    }

    public void test_level1_core_nodechildnodes() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodechildnodes");
    }

    public void test_level1_core_nodechildnodesempty() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodechildnodesempty");
    }

    public void test_level1_core_nodecommentnodeattributes() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodecommentnodeattributes");
    }

    public void test_level1_core_nodecommentnodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodecommentnodename");
    }

    public void test_level1_core_nodecommentnodetype() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodecommentnodetype");
    }

    public void test_level1_core_nodecommentnodevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodecommentnodevalue");
    }

    public void test_level1_core_nodedocumentfragmentnodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodedocumentfragmentnodename");
    }

    public void test_level1_core_nodedocumentfragmentnodetype() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodedocumentfragmentnodetype");
    }

    public void test_level1_core_nodedocumentfragmentnodevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodedocumentfragmentnodevalue");
    }

    public void test_level1_core_nodedocumentnodeattribute() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodedocumentnodeattribute");
    }

    public void test_level1_core_nodedocumentnodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodedocumentnodename");
    }

    public void test_level1_core_nodedocumentnodetype() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodedocumentnodetype");
    }

    public void test_level1_core_nodedocumentnodevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodedocumentnodevalue");
    }

    public void test_level1_core_nodedocumenttypenodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodedocumenttypenodename");
    }

    public void test_level1_core_nodedocumenttypenodetype() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodedocumenttypenodetype");
    }

    public void test_level1_core_nodedocumenttypenodevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodedocumenttypenodevalue");
    }

    public void test_level1_core_nodeelementnodeattributes() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeelementnodeattributes");
    }

    public void test_level1_core_nodeelementnodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeelementnodename");
    }

    public void test_level1_core_nodeelementnodetype() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeelementnodetype");
    }

    public void test_level1_core_nodeelementnodevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeelementnodevalue");
    }

    public void test_level1_core_nodeentityreferencenodeattributes() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeentityreferencenodeattributes");
    }

    public void test_level1_core_nodeentityreferencenodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeentityreferencenodename");
    }

    public void test_level1_core_nodeentityreferencenodetype() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeentityreferencenodetype");
    }

    public void test_level1_core_nodeentityreferencenodevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeentityreferencenodevalue");
    }

    public void test_level1_core_nodegetfirstchild() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodegetfirstchild");
    }

    public void test_level1_core_nodegetfirstchildnull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodegetfirstchildnull");
    }

    public void test_level1_core_nodegetlastchild() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodegetlastchild");
    }

    public void test_level1_core_nodegetlastchildnull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodegetlastchildnull");
    }

    public void test_level1_core_nodegetnextsibling() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodegetnextsibling");
    }

    public void test_level1_core_nodegetnextsiblingnull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodegetnextsiblingnull");
    }

    public void test_level1_core_nodegetownerdocument() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodegetownerdocument");
    }

    public void test_level1_core_nodegetownerdocumentnull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodegetownerdocumentnull");
    }

    public void test_level1_core_nodegetprevioussibling() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodegetprevioussibling");
    }

    public void test_level1_core_nodegetprevioussiblingnull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodegetprevioussiblingnull");
    }

    public void test_level1_core_nodehaschildnodes() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodehaschildnodes");
    }

    public void test_level1_core_nodehaschildnodesfalse() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodehaschildnodesfalse");
    }

    public void test_level1_core_nodeinsertbeforenewchilddiffdocument() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeinsertbeforenewchilddiffdocument");
    }

    public void test_level1_core_nodeinsertbeforenewchildexists() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeinsertbeforenewchildexists");
    }

    public void test_level1_core_nodeinsertbeforenodeancestor() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeinsertbeforenodeancestor");
    }

    public void test_level1_core_nodeinsertbeforenodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeinsertbeforenodename");
    }

    public void test_level1_core_nodelistindexequalzero() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodelistindexequalzero");
    }

    public void test_level1_core_nodelistindexgetlength() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodelistindexgetlength");
    }

    public void test_level1_core_nodelistindexgetlengthofemptylist() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodelistindexgetlengthofemptylist");
    }

    public void test_level1_core_nodelistindexnotzero() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodelistindexnotzero");
    }

    public void test_level1_core_nodelistreturnfirstitem() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodelistreturnfirstitem");
    }

    public void test_level1_core_nodelistreturnlastitem() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodelistreturnlastitem");
    }

    public void test_level1_core_nodelisttraverselist() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodelisttraverselist");
    }

    public void test_level1_core_nodeparentnode() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeparentnode");
    }

    public void test_level1_core_nodeparentnodenull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeparentnodenull");
    }

    public void test_level1_core_nodeprocessinginstructionnodeattributes() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeprocessinginstructionnodeattributes");
    }

    public void test_level1_core_nodeprocessinginstructionnodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeprocessinginstructionnodename");
    }

    public void test_level1_core_nodeprocessinginstructionnodetype() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeprocessinginstructionnodetype");
    }

    public void test_level1_core_nodeprocessinginstructionnodevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodeprocessinginstructionnodevalue");
    }

    public void test_level1_core_noderemovechild() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/noderemovechild");
    }

    public void test_level1_core_noderemovechildgetnodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/noderemovechildgetnodename");
    }

    public void test_level1_core_nodereplacechildnewchilddiffdocument() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodereplacechildnewchilddiffdocument");
    }

    public void test_level1_core_nodereplacechildnodeancestor() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodereplacechildnodeancestor");
    }

    public void test_level1_core_nodereplacechildnodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodereplacechildnodename");
    }

    public void test_level1_core_nodetextnodeattribute() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodetextnodeattribute");
    }

    public void test_level1_core_nodetextnodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodetextnodename");
    }

    public void test_level1_core_nodetextnodetype() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodetextnodetype");
    }

    public void test_level1_core_nodetextnodevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodetextnodevalue");
    }

    public void test_level1_core_processinginstructiongetdata() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/processinginstructiongetdata");
    }

    public void test_level1_core_processinginstructiongettarget() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/processinginstructiongettarget");
    }

    public void test_level1_core_textsplittextfour() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/textsplittextfour");
    }

    public void test_level1_core_textsplittextone() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/textsplittextone");
    }

    public void test_level1_core_textsplittextthree() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/textsplittextthree");
    }

    public void test_level1_core_textwithnomarkup() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/textwithnomarkup");
    }

    public void test_level1_core_nodevalue01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodevalue01");
    }

    public void test_level1_core_nodevalue03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodevalue03");
    }

    public void test_level1_core_nodevalue04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodevalue04");
    }

    public void test_level1_core_nodevalue05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodevalue05");
    }

    public void test_level1_core_nodevalue06() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/nodevalue06");
    }

    public void test_level1_core_hc_attrcreatedocumentfragment() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_attrcreatedocumentfragment");
    }

    public void test_level1_core_hc_attrcreatetextnode() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_attrcreatetextnode");
    }

    public void test_level1_core_hc_attrcreatetextnode2() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_attrcreatetextnode2");
    }

    public void test_level1_core_hc_attreffectivevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_attreffectivevalue");
    }

    public void test_level1_core_hc_attrname() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_attrname");
    }

    public void test_level1_core_hc_attrnextsiblingnull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_attrnextsiblingnull");
    }

    public void test_level1_core_hc_attrparentnodenull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_attrparentnodenull");
    }

    public void test_level1_core_hc_attrprevioussiblingnull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_attrprevioussiblingnull");
    }

    public void test_level1_core_hc_attrspecifiedvalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_attrspecifiedvalue");
    }

    public void test_level1_core_hc_attrspecifiedvaluechanged() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_attrspecifiedvaluechanged");
    }

    public void test_level1_core_hc_characterdataappenddata() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_characterdataappenddata");
    }

    public void test_level1_core_hc_characterdataappenddatagetdata() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_characterdataappenddatagetdata");
    }

    public void test_level1_core_hc_characterdatadeletedatabegining() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_characterdatadeletedatabegining");
    }

    public void test_level1_core_hc_characterdatadeletedataend() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_characterdatadeletedataend");
    }

    public void test_level1_core_hc_characterdatadeletedataexceedslength() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_characterdatadeletedataexceedslength");
    }

    public void test_level1_core_hc_characterdatadeletedatagetlengthanddata() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_characterdatadeletedatagetlengthanddata");
    }

    public void test_level1_core_hc_characterdatadeletedatamiddle() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_characterdatadeletedatamiddle");
    }

    public void test_level1_core_hc_characterdatagetdata() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_characterdatagetdata");
    }

    public void test_level1_core_hc_characterdatagetlength() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_characterdatagetlength");
    }

    public void test_level1_core_hc_characterdatainsertdatabeginning() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_characterdatainsertdatabeginning");
    }

    public void test_level1_core_hc_characterdatainsertdataend() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_characterdatainsertdataend");
    }

    public void test_level1_core_hc_characterdatainsertdatamiddle() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_characterdatainsertdatamiddle");
    }

    public void test_level1_core_hc_characterdatareplacedatabegining() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_characterdatareplacedatabegining");
    }

    public void test_level1_core_hc_characterdatareplacedataend() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_characterdatareplacedataend");
    }

    public void test_level1_core_hc_characterdatareplacedataexceedslengthofarg() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_characterdatareplacedataexceedslengthofarg");
    }

    public void test_level1_core_hc_characterdatareplacedataexceedslengthofdata() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_characterdatareplacedataexceedslengthofdata");
    }

    public void test_level1_core_hc_characterdatareplacedatamiddle() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_characterdatareplacedatamiddle");
    }

    public void test_level1_core_hc_characterdatasubstringvalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_characterdatasubstringvalue");
    }

    public void test_level1_core_hc_commentgetcomment() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_commentgetcomment");
    }

    public void test_level1_core_hc_documentcreatecomment() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_documentcreatecomment");
    }

    public void test_level1_core_hc_documentcreatedocumentfragment() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_documentcreatedocumentfragment");
    }

    public void test_level1_core_hc_documentcreateelement() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_documentcreateelement");
    }

    public void test_level1_core_hc_documentcreateelementcasesensitive() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_documentcreateelementcasesensitive");
    }

    public void test_level1_core_hc_documentcreatetextnode() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_documentcreatetextnode");
    }

    public void test_level1_core_hc_documentgetdoctype() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_documentgetdoctype");
    }

    public void test_level1_core_hc_documentgetelementsbytagnamelength() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_documentgetelementsbytagnamelength");
    }

    public void test_level1_core_hc_documentgetelementsbytagnametotallength() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_documentgetelementsbytagnametotallength");
    }

    public void test_level1_core_hc_documentgetelementsbytagnamevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_documentgetelementsbytagnamevalue");
    }

    public void test_level1_core_hc_documentgetimplementation() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_documentgetimplementation");
    }

    public void test_level1_core_hc_documentgetrootnode() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_documentgetrootnode");
    }

    public void test_level1_core_hc_documentinvalidcharacterexceptioncreateattribute() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_documentinvalidcharacterexceptioncreateattribute");
    }

    public void test_level1_core_hc_documentinvalidcharacterexceptioncreateattribute1() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_documentinvalidcharacterexceptioncreateattribute1");
    }

    public void test_level1_core_hc_documentinvalidcharacterexceptioncreateelement() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_documentinvalidcharacterexceptioncreateelement");
    }

    public void test_level1_core_hc_documentinvalidcharacterexceptioncreateelement1() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_documentinvalidcharacterexceptioncreateelement1");
    }

    public void test_level1_core_hc_domimplementationfeaturenoversion() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_domimplementationfeaturenoversion");
    }

    public void test_level1_core_hc_domimplementationfeaturenull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_domimplementationfeaturenull");
    }

    public void test_level1_core_hc_domimplementationfeaturexml() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_domimplementationfeaturexml");
    }

    public void test_level1_core_hc_elementaddnewattribute() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_elementaddnewattribute");
    }

    public void test_level1_core_hc_elementassociatedattribute() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_elementassociatedattribute");
    }

    public void test_level1_core_hc_elementchangeattributevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_elementchangeattributevalue");
    }

    public void test_level1_core_hc_elementgetattributenode() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_elementgetattributenode");
    }

    public void test_level1_core_hc_elementgetattributenodenull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_elementgetattributenodenull");
    }

    public void test_level1_core_hc_elementgetelementsbytagname() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_elementgetelementsbytagname");
    }

    public void test_level1_core_hc_elementgetelementsbytagnameaccessnodelist() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_elementgetelementsbytagnameaccessnodelist");
    }

    public void test_level1_core_hc_elementgetelementsbytagnamenomatch() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_elementgetelementsbytagnamenomatch");
    }

    public void test_level1_core_hc_elementgettagname() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_elementgettagname");
    }

    public void test_level1_core_hc_elementinuseattributeerr() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_elementinuseattributeerr");
    }

    public void test_level1_core_hc_elementinvalidcharacterexception() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_elementinvalidcharacterexception");
    }

    public void test_level1_core_hc_elementinvalidcharacterexception1() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_elementinvalidcharacterexception1");
    }

    public void test_level1_core_hc_elementnormalize() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_elementnormalize");
    }

    public void test_level1_core_hc_elementnotfounderr() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_elementnotfounderr");
    }

    public void test_level1_core_hc_elementremoveattribute() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_elementremoveattribute");
    }

    public void test_level1_core_hc_elementremoveattributeaftercreate() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_elementremoveattributeaftercreate");
    }

    public void test_level1_core_hc_elementremoveattributenode() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_elementremoveattributenode");
    }

    public void test_level1_core_hc_elementreplaceexistingattributegevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_elementreplaceexistingattributegevalue");
    }

    public void test_level1_core_hc_elementretrieveattrvalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_elementretrieveattrvalue");
    }

    public void test_level1_core_hc_elementretrievetagname() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_elementretrievetagname");
    }

    public void test_level1_core_hc_elementsetattributenodenull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_elementsetattributenodenull");
    }

    public void test_level1_core_hc_elementwrongdocumenterr() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_elementwrongdocumenterr");
    }

    public void test_level1_core_hc_namednodemapgetnameditem() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_namednodemapgetnameditem");
    }

    public void test_level1_core_hc_namednodemapinuseattributeerr() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_namednodemapinuseattributeerr");
    }

    public void test_level1_core_hc_namednodemapnotfounderr() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_namednodemapnotfounderr");
    }

    public void test_level1_core_hc_namednodemapremovenameditem() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_namednodemapremovenameditem");
    }

    public void test_level1_core_hc_namednodemapreturnattrnode() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_namednodemapreturnattrnode");
    }

    public void test_level1_core_hc_namednodemapreturnnull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_namednodemapreturnnull");
    }

    public void test_level1_core_hc_namednodemapsetnameditem() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_namednodemapsetnameditem");
    }

    public void test_level1_core_hc_namednodemapsetnameditemreturnvalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_namednodemapsetnameditemreturnvalue");
    }

    public void test_level1_core_hc_namednodemapsetnameditemwithnewvalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_namednodemapsetnameditemwithnewvalue");
    }

    public void test_level1_core_hc_namednodemapwrongdocumenterr() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_namednodemapwrongdocumenterr");
    }

    public void test_level1_core_hc_nodeappendchild() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodeappendchild");
    }

    public void test_level1_core_hc_nodeappendchildgetnodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodeappendchildgetnodename");
    }

    public void test_level1_core_hc_nodeappendchildnewchilddiffdocument() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodeappendchildnewchilddiffdocument");
    }

    public void test_level1_core_hc_nodeappendchildnodeancestor() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodeappendchildnodeancestor");
    }

    public void test_level1_core_hc_nodeattributenodeattribute() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodeattributenodeattribute");
    }

    public void test_level1_core_hc_nodeattributenodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodeattributenodename");
    }

    public void test_level1_core_hc_nodeattributenodetype() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodeattributenodetype");
    }

    public void test_level1_core_hc_nodeattributenodevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodeattributenodevalue");
    }

    public void test_level1_core_hc_nodechildnodes() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodechildnodes");
    }

    public void test_level1_core_hc_nodechildnodesempty() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodechildnodesempty");
    }

    public void test_level1_core_hc_nodecommentnodeattributes() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodecommentnodeattributes");
    }

    public void test_level1_core_hc_nodecommentnodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodecommentnodename");
    }

    public void test_level1_core_hc_nodecommentnodetype() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodecommentnodetype");
    }

    public void test_level1_core_hc_nodecommentnodevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodecommentnodevalue");
    }

    public void test_level1_core_hc_nodedocumentfragmentnodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodedocumentfragmentnodename");
    }

    public void test_level1_core_hc_nodedocumentfragmentnodetype() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodedocumentfragmentnodetype");
    }

    public void test_level1_core_hc_nodedocumentfragmentnodevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodedocumentfragmentnodevalue");
    }

    public void test_level1_core_hc_nodedocumentnodeattribute() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodedocumentnodeattribute");
    }

    public void test_level1_core_hc_nodedocumentnodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodedocumentnodename");
    }

    public void test_level1_core_hc_nodedocumentnodetype() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodedocumentnodetype");
    }

    public void test_level1_core_hc_nodedocumentnodevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodedocumentnodevalue");
    }

    public void test_level1_core_hc_nodeelementnodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodeelementnodename");
    }

    public void test_level1_core_hc_nodeelementnodetype() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodeelementnodetype");
    }

    public void test_level1_core_hc_nodeelementnodevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodeelementnodevalue");
    }

    public void test_level1_core_hc_nodegetfirstchild() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodegetfirstchild");
    }

    public void test_level1_core_hc_nodegetfirstchildnull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodegetfirstchildnull");
    }

    public void test_level1_core_hc_nodegetlastchild() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodegetlastchild");
    }

    public void test_level1_core_hc_nodegetlastchildnull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodegetlastchildnull");
    }

    public void test_level1_core_hc_nodegetnextsibling() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodegetnextsibling");
    }

    public void test_level1_core_hc_nodegetnextsiblingnull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodegetnextsiblingnull");
    }

    public void test_level1_core_hc_nodegetownerdocument() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodegetownerdocument");
    }

    public void test_level1_core_hc_nodegetownerdocumentnull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodegetownerdocumentnull");
    }

    public void test_level1_core_hc_nodegetprevioussibling() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodegetprevioussibling");
    }

    public void test_level1_core_hc_nodegetprevioussiblingnull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodegetprevioussiblingnull");
    }

    public void test_level1_core_hc_nodehaschildnodes() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodehaschildnodes");
    }

    public void test_level1_core_hc_nodehaschildnodesfalse() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodehaschildnodesfalse");
    }

    public void test_level1_core_hc_nodeinsertbeforenewchilddiffdocument() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodeinsertbeforenewchilddiffdocument");
    }

    public void test_level1_core_hc_nodeinsertbeforenodeancestor() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodeinsertbeforenodeancestor");
    }

    public void test_level1_core_hc_nodeinsertbeforenodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodeinsertbeforenodename");
    }

    public void test_level1_core_hc_nodelistindexequalzero() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodelistindexequalzero");
    }

    public void test_level1_core_hc_nodelistindexgetlength() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodelistindexgetlength");
    }

    public void test_level1_core_hc_nodelistindexgetlengthofemptylist() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodelistindexgetlengthofemptylist");
    }

    public void test_level1_core_hc_nodelistindexnotzero() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodelistindexnotzero");
    }

    public void test_level1_core_hc_nodelistreturnfirstitem() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodelistreturnfirstitem");
    }

    public void test_level1_core_hc_nodelistreturnlastitem() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodelistreturnlastitem");
    }

    public void test_level1_core_hc_nodelisttraverselist() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodelisttraverselist");
    }

    public void test_level1_core_hc_nodeparentnode() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodeparentnode");
    }

    public void test_level1_core_hc_nodeparentnodenull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodeparentnodenull");
    }

    public void test_level1_core_hc_noderemovechild() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_noderemovechild");
    }

    public void test_level1_core_hc_noderemovechildgetnodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_noderemovechildgetnodename");
    }

    public void test_level1_core_hc_nodereplacechildnewchilddiffdocument() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodereplacechildnewchilddiffdocument");
    }

    public void test_level1_core_hc_nodereplacechildnodeancestor() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodereplacechildnodeancestor");
    }

    public void test_level1_core_hc_nodereplacechildnodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodereplacechildnodename");
    }

    public void test_level1_core_hc_nodetextnodeattribute() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodetextnodeattribute");
    }

    public void test_level1_core_hc_nodetextnodename() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodetextnodename");
    }

    public void test_level1_core_hc_nodetextnodetype() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodetextnodetype");
    }

    public void test_level1_core_hc_nodetextnodevalue() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodetextnodevalue");
    }

    public void test_level1_core_hc_nodevalue01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodevalue01");
    }

    public void test_level1_core_hc_nodevalue03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodevalue03");
    }

    public void test_level1_core_hc_nodevalue04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodevalue04");
    }

    public void test_level1_core_hc_nodevalue05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodevalue05");
    }

    public void test_level1_core_hc_nodevalue06() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_nodevalue06");
    }

    public void test_level1_core_hc_textsplittextfour() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_textsplittextfour");
    }

    public void test_level1_core_hc_textsplittextone() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_textsplittextone");
    }

    public void test_level1_core_hc_textsplittextthree() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_textsplittextthree");
    }

    public void test_level1_core_hc_textwithnomarkup() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_textwithnomarkup");
    }

    public void test_level1_core_hc_attrappendchild2() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_attrappendchild2");
    }

    public void test_level1_core_hc_attrappendchild4() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_attrappendchild4");
    }

    public void test_level1_core_hc_attrinsertbefore5() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_attrinsertbefore5");
    }

    public void test_level1_core_hc_attrinsertbefore7() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level1/core/hc_attrinsertbefore7");
    }

    public void test_level2_core_attrgetownerelement02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/attrgetownerelement02");
    }

    public void test_level2_core_attrgetownerelement03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/attrgetownerelement03");
    }

    public void test_level2_core_attrgetownerelement04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/attrgetownerelement04");
    }

    public void test_level2_core_attrgetownerelement05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/attrgetownerelement05");
    }

    public void test_level2_core_createAttributeNS01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/createAttributeNS01");
    }

    public void test_level2_core_createAttributeNS02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/createAttributeNS02");
    }

    public void test_level2_core_createAttributeNS03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/createAttributeNS03");
    }

    public void test_level2_core_createAttributeNS04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/createAttributeNS04");
    }

    public void test_level2_core_createAttributeNS05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/createAttributeNS05");
    }

    public void test_level2_core_createDocument01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/createDocument01");
    }

    public void test_level2_core_createDocument02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/createDocument02");
    }

    public void test_level2_core_createDocument05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/createDocument05");
    }

    public void test_level2_core_createDocument06() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/createDocument06");
    }

    public void test_level2_core_createDocument07() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/createDocument07");
    }

    public void test_level2_core_createDocumentType01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/createDocumentType01");
    }

    public void test_level2_core_createDocumentType02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/createDocumentType02");
    }

    public void test_level2_core_createDocumentType03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/createDocumentType03");
    }

    public void test_level2_core_createElementNS01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/createElementNS01");
    }

    public void test_level2_core_createElementNS02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/createElementNS02");
    }

    public void test_level2_core_createElementNS03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/createElementNS03");
    }

    public void test_level2_core_createElementNS04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/createElementNS04");
    }

    public void test_level2_core_createElementNS05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/createElementNS05");
    }

    public void test_level2_core_documentcreateattributeNS01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentcreateattributeNS01");
    }

    public void test_level2_core_documentcreateattributeNS02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentcreateattributeNS02");
    }

    public void test_level2_core_documentcreateattributeNS03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentcreateattributeNS03");
    }

    public void test_level2_core_documentcreateattributeNS04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentcreateattributeNS04");
    }

    public void test_level2_core_documentcreateattributeNS05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentcreateattributeNS05");
    }

    public void test_level2_core_documentcreateattributeNS06() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentcreateattributeNS06");
    }

    public void test_level2_core_documentcreateattributeNS07() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentcreateattributeNS07");
    }

    public void test_level2_core_documentcreateelementNS01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentcreateelementNS01");
    }

    public void test_level2_core_documentcreateelementNS02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentcreateelementNS02");
    }

    public void test_level2_core_documentcreateelementNS05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentcreateelementNS05");
    }

    public void test_level2_core_documentcreateelementNS06() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentcreateelementNS06");
    }

    public void test_level2_core_documentgetelementbyid01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentgetelementbyid01");
    }

    public void test_level2_core_documentgetelementsbytagnameNS01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentgetelementsbytagnameNS01");
    }

    public void test_level2_core_documentgetelementsbytagnameNS02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentgetelementsbytagnameNS02");
    }

    public void test_level2_core_documentgetelementsbytagnameNS03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentgetelementsbytagnameNS03");
    }

    public void test_level2_core_documentgetelementsbytagnameNS04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentgetelementsbytagnameNS04");
    }

    public void test_level2_core_documentgetelementsbytagnameNS05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentgetelementsbytagnameNS05");
    }

    public void test_level2_core_documentimportnode02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentimportnode02");
    }

    public void test_level2_core_documentimportnode05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentimportnode05");
    }

    public void test_level2_core_documentimportnode06() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentimportnode06");
    }

    public void test_level2_core_documentimportnode07() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentimportnode07");
    }

    public void test_level2_core_documentimportnode08() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentimportnode08");
    }

    public void test_level2_core_documentimportnode09() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentimportnode09");
    }

    public void test_level2_core_documentimportnode10() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentimportnode10");
    }

    public void test_level2_core_documentimportnode11() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentimportnode11");
    }

    public void test_level2_core_documentimportnode12() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentimportnode12");
    }

    public void test_level2_core_documentimportnode13() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentimportnode13");
    }

    public void test_level2_core_documentimportnode15() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentimportnode15");
    }

    public void test_level2_core_documentimportnode17() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentimportnode17");
    }

    public void test_level2_core_documentimportnode18() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documentimportnode18");
    }

    public void test_level2_core_documenttypeinternalSubset01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documenttypeinternalSubset01");
    }

    public void test_level2_core_documenttypepublicid01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documenttypepublicid01");
    }

    public void test_level2_core_documenttypesystemid01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/documenttypesystemid01");
    }

    public void test_level2_core_domimplementationcreatedocument03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/domimplementationcreatedocument03");
    }

    public void test_level2_core_domimplementationcreatedocument04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/domimplementationcreatedocument04");
    }

    public void test_level2_core_domimplementationcreatedocument05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/domimplementationcreatedocument05");
    }

    public void test_level2_core_domimplementationcreatedocument07() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/domimplementationcreatedocument07");
    }

    public void test_level2_core_domimplementationcreatedocumenttype01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/domimplementationcreatedocumenttype01");
    }

    public void test_level2_core_domimplementationcreatedocumenttype02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/domimplementationcreatedocumenttype02");
    }

    public void test_level2_core_domimplementationcreatedocumenttype04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/domimplementationcreatedocumenttype04");
    }

    public void test_level2_core_domimplementationfeaturecore() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/domimplementationfeaturecore");
    }

    public void test_level2_core_domimplementationfeaturexmlversion2() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/domimplementationfeaturexmlversion2");
    }

    public void test_level2_core_domimplementationhasfeature01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/domimplementationhasfeature01");
    }

    public void test_level2_core_domimplementationhasfeature02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/domimplementationhasfeature02");
    }

    public void test_level2_core_elementgetattributenodens01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/elementgetattributenodens01");
    }

    public void test_level2_core_elementgetattributenodens02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/elementgetattributenodens02");
    }

    public void test_level2_core_elementgetelementsbytagnamens02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/elementgetelementsbytagnamens02");
    }

    public void test_level2_core_elementgetelementsbytagnamens04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/elementgetelementsbytagnamens04");
    }

    public void test_level2_core_elementgetelementsbytagnamens05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/elementgetelementsbytagnamens05");
    }

    public void test_level2_core_elementhasattribute01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/elementhasattribute01");
    }

    public void test_level2_core_elementhasattribute03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/elementhasattribute03");
    }

    public void test_level2_core_elementhasattribute04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/elementhasattribute04");
    }

    public void test_level2_core_elementhasattributens02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/elementhasattributens02");
    }

    public void test_level2_core_elementhasattributens03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/elementhasattributens03");
    }

    public void test_level2_core_elementremoveattributens01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/elementremoveattributens01");
    }

    public void test_level2_core_elementsetattributenodens01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/elementsetattributenodens01");
    }

    public void test_level2_core_elementsetattributenodens02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/elementsetattributenodens02");
    }

    public void test_level2_core_elementsetattributenodens03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/elementsetattributenodens03");
    }

    public void test_level2_core_elementsetattributenodens04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/elementsetattributenodens04");
    }

    public void test_level2_core_elementsetattributenodens05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/elementsetattributenodens05");
    }

    public void test_level2_core_elementsetattributens01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/elementsetattributens01");
    }

    public void test_level2_core_elementsetattributens02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/elementsetattributens02");
    }

    public void test_level2_core_elementsetattributens03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/elementsetattributens03");
    }

    public void test_level2_core_elementsetattributens04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/elementsetattributens04");
    }

    public void test_level2_core_elementsetattributens05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/elementsetattributens05");
    }

    public void test_level2_core_elementsetattributens08() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/elementsetattributens08");
    }

    public void test_level2_core_elementsetattributensurinull() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/elementsetattributensurinull");
    }

    public void test_level2_core_getAttributeNS02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/getAttributeNS02");
    }

    public void test_level2_core_getAttributeNS03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/getAttributeNS03");
    }

    public void test_level2_core_getAttributeNS04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/getAttributeNS04");
    }

    public void test_level2_core_getAttributeNS05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/getAttributeNS05");
    }

    public void test_level2_core_getAttributeNodeNS01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/getAttributeNodeNS01");
    }

    public void test_level2_core_getAttributeNodeNS02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/getAttributeNodeNS02");
    }

    public void test_level2_core_getElementById02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/getElementById02");
    }

    public void test_level2_core_getElementsByTagNameNS02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/getElementsByTagNameNS02");
    }

    public void test_level2_core_getElementsByTagNameNS03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/getElementsByTagNameNS03");
    }

    public void test_level2_core_getElementsByTagNameNS04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/getElementsByTagNameNS04");
    }

    public void test_level2_core_getElementsByTagNameNS05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/getElementsByTagNameNS05");
    }

    public void test_level2_core_getElementsByTagNameNS06() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/getElementsByTagNameNS06");
    }

    public void test_level2_core_getElementsByTagNameNS07() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/getElementsByTagNameNS07");
    }

    public void test_level2_core_getElementsByTagNameNS08() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/getElementsByTagNameNS08");
    }

    public void test_level2_core_getElementsByTagNameNS09() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/getElementsByTagNameNS09");
    }

    public void test_level2_core_getElementsByTagNameNS10() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/getElementsByTagNameNS10");
    }

    public void test_level2_core_getElementsByTagNameNS11() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/getElementsByTagNameNS11");
    }

    public void test_level2_core_getElementsByTagNameNS12() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/getElementsByTagNameNS12");
    }

    public void test_level2_core_getElementsByTagNameNS13() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/getElementsByTagNameNS13");
    }

    public void test_level2_core_getElementsByTagNameNS14() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/getElementsByTagNameNS14");
    }

    public void test_level2_core_getNamedItemNS01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/getNamedItemNS01");
    }

    public void test_level2_core_getNamedItemNS02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/getNamedItemNS02");
    }

    public void test_level2_core_hasAttribute01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/hasAttribute01");
    }

    public void test_level2_core_hasAttribute03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/hasAttribute03");
    }

    public void test_level2_core_hasAttributeNS01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/hasAttributeNS01");
    }

    public void test_level2_core_hasAttributeNS02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/hasAttributeNS02");
    }

    public void test_level2_core_hasAttributeNS03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/hasAttributeNS03");
    }

    public void test_level2_core_hasAttributeNS05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/hasAttributeNS05");
    }

    public void test_level2_core_hasAttributes01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/hasAttributes01");
    }

    public void test_level2_core_hasAttributes02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/hasAttributes02");
    }

    public void test_level2_core_hc_namednodemapinvalidtype1() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/hc_namednodemapinvalidtype1");
    }

    public void test_level2_core_hc_nodedocumentfragmentnormalize1() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/hc_nodedocumentfragmentnormalize1");
    }

    public void test_level2_core_hc_nodedocumentfragmentnormalize2() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/hc_nodedocumentfragmentnormalize2");
    }

    public void test_level2_core_importNode02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/importNode02");
    }

    public void test_level2_core_importNode03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/importNode03");
    }

    public void test_level2_core_importNode04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/importNode04");
    }

    public void test_level2_core_importNode08() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/importNode08");
    }

    public void test_level2_core_importNode10() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/importNode10");
    }

    public void test_level2_core_importNode14() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/importNode14");
    }

    public void test_level2_core_importNode15() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/importNode15");
    }

    public void test_level2_core_importNode16() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/importNode16");
    }

    public void test_level2_core_importNode17() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/importNode17");
    }

    public void test_level2_core_internalSubset01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/internalSubset01");
    }

    public void test_level2_core_isSupported01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/isSupported01");
    }

    public void test_level2_core_isSupported02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/isSupported02");
    }

    public void test_level2_core_isSupported04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/isSupported04");
    }

    public void test_level2_core_isSupported05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/isSupported05");
    }

    public void test_level2_core_isSupported06() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/isSupported06");
    }

    public void test_level2_core_isSupported07() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/isSupported07");
    }

    public void test_level2_core_isSupported09() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/isSupported09");
    }

    public void test_level2_core_isSupported10() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/isSupported10");
    }

    public void test_level2_core_isSupported11() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/isSupported11");
    }

    public void test_level2_core_isSupported12() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/isSupported12");
    }

    public void test_level2_core_isSupported13() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/isSupported13");
    }

    public void test_level2_core_isSupported14() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/isSupported14");
    }

    public void test_level2_core_localName01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/localName01");
    }

    public void test_level2_core_localName02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/localName02");
    }

    public void test_level2_core_localName03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/localName03");
    }

    public void test_level2_core_localName04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/localName04");
    }

    public void test_level2_core_namednodemapgetnameditemns02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/namednodemapgetnameditemns02");
    }

    public void test_level2_core_namednodemapgetnameditemns03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/namednodemapgetnameditemns03");
    }

    public void test_level2_core_namednodemapgetnameditemns04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/namednodemapgetnameditemns04");
    }

    public void test_level2_core_namednodemapgetnameditemns05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/namednodemapgetnameditemns05");
    }

    public void test_level2_core_namednodemapgetnameditemns06() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/namednodemapgetnameditemns06");
    }

    public void test_level2_core_namednodemapremovenameditemns01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/namednodemapremovenameditemns01");
    }

    public void test_level2_core_namednodemapremovenameditemns03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/namednodemapremovenameditemns03");
    }

    public void test_level2_core_namednodemapremovenameditemns06() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/namednodemapremovenameditemns06");
    }

    public void test_level2_core_namednodemapremovenameditemns07() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/namednodemapremovenameditemns07");
    }

    public void test_level2_core_namednodemapremovenameditemns08() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/namednodemapremovenameditemns08");
    }

    public void test_level2_core_namednodemapremovenameditemns09() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/namednodemapremovenameditemns09");
    }

    public void test_level2_core_namednodemapsetnameditemns01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/namednodemapsetnameditemns01");
    }

    public void test_level2_core_namednodemapsetnameditemns02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/namednodemapsetnameditemns02");
    }

    public void test_level2_core_namednodemapsetnameditemns03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/namednodemapsetnameditemns03");
    }

    public void test_level2_core_namednodemapsetnameditemns04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/namednodemapsetnameditemns04");
    }

    public void test_level2_core_namednodemapsetnameditemns06() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/namednodemapsetnameditemns06");
    }

    public void test_level2_core_namednodemapsetnameditemns07() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/namednodemapsetnameditemns07");
    }

    public void test_level2_core_namednodemapsetnameditemns08() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/namednodemapsetnameditemns08");
    }

    public void test_level2_core_namespaceURI02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/namespaceURI02");
    }

    public void test_level2_core_namespaceURI03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/namespaceURI03");
    }

    public void test_level2_core_namespaceURI04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/namespaceURI04");
    }

    public void test_level2_core_nodegetlocalname03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/nodegetlocalname03");
    }

    public void test_level2_core_nodegetnamespaceuri03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/nodegetnamespaceuri03");
    }

    public void test_level2_core_nodegetownerdocument01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/nodegetownerdocument01");
    }

    public void test_level2_core_nodegetownerdocument02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/nodegetownerdocument02");
    }

    public void test_level2_core_nodegetprefix03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/nodegetprefix03");
    }

    public void test_level2_core_nodehasattributes01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/nodehasattributes01");
    }

    public void test_level2_core_nodehasattributes02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/nodehasattributes02");
    }

    public void test_level2_core_nodehasattributes03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/nodehasattributes03");
    }

    public void test_level2_core_nodehasattributes04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/nodehasattributes04");
    }

    public void test_level2_core_nodeissupported01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/nodeissupported01");
    }

    public void test_level2_core_nodeissupported02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/nodeissupported02");
    }

    public void test_level2_core_nodeissupported03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/nodeissupported03");
    }

    public void test_level2_core_nodeissupported04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/nodeissupported04");
    }

    public void test_level2_core_nodeissupported05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/nodeissupported05");
    }

    public void test_level2_core_nodenormalize01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/nodenormalize01");
    }

    public void test_level2_core_nodesetprefix01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/nodesetprefix01");
    }

    public void test_level2_core_nodesetprefix02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/nodesetprefix02");
    }

    public void test_level2_core_nodesetprefix03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/nodesetprefix03");
    }

    public void test_level2_core_nodesetprefix05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/nodesetprefix05");
    }

    public void test_level2_core_nodesetprefix06() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/nodesetprefix06");
    }

    public void test_level2_core_nodesetprefix07() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/nodesetprefix07");
    }

    public void test_level2_core_nodesetprefix08() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/nodesetprefix08");
    }

    public void test_level2_core_normalize01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/normalize01");
    }

    public void test_level2_core_ownerDocument01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/ownerDocument01");
    }

    public void test_level2_core_ownerElement01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/ownerElement01");
    }

    public void test_level2_core_ownerElement02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/ownerElement02");
    }

    public void test_level2_core_prefix01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/prefix01");
    }

    public void test_level2_core_prefix02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/prefix02");
    }

    public void test_level2_core_prefix03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/prefix03");
    }

    public void test_level2_core_prefix04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/prefix04");
    }

    public void test_level2_core_prefix05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/prefix05");
    }

    public void test_level2_core_prefix07() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/prefix07");
    }

    public void test_level2_core_prefix10() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/prefix10");
    }

    public void test_level2_core_prefix11() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/prefix11");
    }

    public void test_level2_core_publicId01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/publicId01");
    }

    public void test_level2_core_removeNamedItemNS01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/removeNamedItemNS01");
    }

    public void test_level2_core_removeNamedItemNS02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/removeNamedItemNS02");
    }

    public void test_level2_core_setAttributeNS01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/setAttributeNS01");
    }

    public void test_level2_core_setAttributeNS02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/setAttributeNS02");
    }

    public void test_level2_core_setAttributeNS04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/setAttributeNS04");
    }

    public void test_level2_core_setAttributeNS05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/setAttributeNS05");
    }

    public void test_level2_core_setAttributeNS06() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/setAttributeNS06");
    }

    public void test_level2_core_setAttributeNS07() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/setAttributeNS07");
    }

    public void test_level2_core_setAttributeNS09() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/setAttributeNS09");
    }

    public void test_level2_core_setAttributeNodeNS01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/setAttributeNodeNS01");
    }

    public void test_level2_core_setAttributeNodeNS03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/setAttributeNodeNS03");
    }

    public void test_level2_core_setAttributeNodeNS04() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/setAttributeNodeNS04");
    }

    public void test_level2_core_setAttributeNodeNS05() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/setAttributeNodeNS05");
    }

    public void test_level2_core_setNamedItemNS01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/setNamedItemNS01");
    }

    public void test_level2_core_setNamedItemNS02() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/setNamedItemNS02");
    }

    public void test_level2_core_setNamedItemNS03() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/setNamedItemNS03");
    }

    public void test_level2_core_systemId01() throws Throwable { 
        runDomTest("http://www.w3.org/2001/DOM-Test-Suite/level2/core/systemId01");
    }

}
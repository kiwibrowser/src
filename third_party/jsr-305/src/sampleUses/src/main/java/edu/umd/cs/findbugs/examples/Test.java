package edu.umd.cs.findbugs.examples;

import javax.annotation.meta.When;

import edu.umd.cs.findbugs.DottedClassName;
import edu.umd.cs.findbugs.SlashedClassName;

public class Test {
	
	
	public void foo(@SlashedClassName String foo) {}
	
	public void foo2(@DottedClassName String foo) {
		foo(foo); // should get warning here
	}

	public void foo3(String foo) {
		foo(foo); 
	}
	public void foo4(@DottedClassName String foo) {
		foo3(foo); 
	}
	
	public void foo5(@SlashedClassName(when=When.MAYBE) String foo) {
		foo(foo); 
	}
	public void foo6(@SlashedClassName(when=When.UNKNOWN) String foo) {
		foo(foo); 
	}
}


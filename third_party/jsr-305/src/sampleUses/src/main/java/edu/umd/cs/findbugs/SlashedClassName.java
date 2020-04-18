package edu.umd.cs.findbugs;

import java.lang.annotation.Documented;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

import javax.annotation.meta.TypeQualifier;
import javax.annotation.meta.When;

@Documented
@TypeQualifier(applicableTo=CharSequence.class)
@Retention(RetentionPolicy.RUNTIME)
public @interface SlashedClassName {
	When when() default When.ALWAYS;
}

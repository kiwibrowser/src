package edu.umd.cs.findbugs;

import java.lang.annotation.Documented;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

import javax.annotation.meta.TypeQualifierNickname;
import javax.annotation.meta.When;

@Documented
@SlashedClassName(when=When.NEVER)
@TypeQualifierNickname
@Retention(RetentionPolicy.RUNTIME)
public @interface DottedClassName { }
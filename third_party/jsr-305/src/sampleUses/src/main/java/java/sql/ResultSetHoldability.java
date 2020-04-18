package java.sql;

import java.lang.annotation.Documented;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

import javax.annotation.meta.TypeQualifier;

@Documented
@TypeQualifier(applicableTo=Integer.class)
@Retention(RetentionPolicy.RUNTIME)
public @interface ResultSetHoldability {

}

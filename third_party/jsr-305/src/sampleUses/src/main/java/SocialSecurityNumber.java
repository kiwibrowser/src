import java.lang.annotation.Documented;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

import javax.annotation.MatchesPattern;
import javax.annotation.meta.TypeQualifierNickname;

@Documented
@TypeQualifierNickname
@MatchesPattern("[0-9]{3}-[0-9]{2}-[0-9]{4}")
@Retention(RetentionPolicy.RUNTIME)
public @interface SocialSecurityNumber {
}

import java.lang.annotation.Documented;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

import javax.annotation.MatchesPattern;
import javax.annotation.meta.TypeQualifier;
import javax.annotation.meta.TypeQualifierValidator;
import javax.annotation.meta.When;

@Documented
@TypeQualifier
@MatchesPattern("[0-9]{16}")
@Retention(RetentionPolicy.RUNTIME)
public @interface CreditCardNumber {
	class Checker implements TypeQualifierValidator<CreditCardNumber> {

		public When forConstantValue(CreditCardNumber annotation, Object v) {
			if (!(v instanceof String))
				return When.NEVER;
			String s = (String) v;
			if (LuhnVerification.checkNumber(s))
				return When.ALWAYS;
			return When.NEVER;
		}
	}
}

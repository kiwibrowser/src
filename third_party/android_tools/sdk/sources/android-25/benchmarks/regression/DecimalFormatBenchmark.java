package benchmarks.regression;

import java.math.BigDecimal;
import java.text.DecimalFormat;
import java.text.NumberFormat;
import java.util.Locale;

public class DecimalFormatBenchmark {

    private static final String EXP_PATTERN = "##E0";

    private static final DecimalFormat df = (DecimalFormat) DecimalFormat.getInstance();
    // Keep patternInstance for timing with patterns, to not dirty the plain instance.
    private static final DecimalFormat patternInstance = (DecimalFormat)
            DecimalFormat.getInstance();
    private static final DecimalFormat dfCurrencyUS = (DecimalFormat)
            NumberFormat.getCurrencyInstance(Locale.US);
    private static final DecimalFormat dfCurrencyFR = (DecimalFormat)
            NumberFormat.getInstance(Locale.FRANCE);

    private static final BigDecimal BD10E3 = new BigDecimal("10E3");
    private static final BigDecimal BD10E9 = new BigDecimal("10E9");
    private static final BigDecimal BD10E100 = new BigDecimal("10E100");
    private static final BigDecimal BD10E1000 = new BigDecimal("10E1000");

    private static final int WHOLE_NUMBER = 10;
    private static final double TWO_DP_NUMBER = 3.14;

    public static void formatWithGrouping(Object obj, int reps) {
        df.setGroupingSize(3);
        df.setGroupingUsed(true);
        for (int i = 0; i < reps; i++) {
            df.format(obj);
        }
    }

    public static void format(String pattern, Object obj, int reps) {
        patternInstance.applyPattern(pattern);
        for (int i = 0; i < reps; i++) {
            patternInstance.format(obj);
        }
    }

    public static void format(Object obj, int reps) {
        for (int i = 0; i < reps; i++) {
            df.format(obj);
        }
    }

    public static void formatToCharacterIterator(Object obj, int reps) {
        for (int i = 0; i < reps; i++) {
            df.formatToCharacterIterator(obj);
        }
    }


    public static void formatCurrencyUS(Object obj, int reps) {
        for (int i = 0; i < reps; i++) {
            dfCurrencyUS.format(obj);
        }
    }

    public static void formatCurrencyFR(Object obj, int reps) {
        for (int i = 0; i < reps; i++) {
            dfCurrencyFR.format(obj);
        }
    }

    public void time_formatGrouping_BigDecimal10e3(int reps) {
        formatWithGrouping(BD10E3, reps);
    }

    public void time_formatGrouping_BigDecimal10e9(int reps) {
        formatWithGrouping(BD10E9, reps);
    }

    public void time_formatGrouping_BigDecimal10e100(int reps) {
        formatWithGrouping(BD10E100, reps);
    }

    public void time_formatGrouping_BigDecimal10e1000(int reps) {
        formatWithGrouping(BD10E1000, reps);
    }

    public void time_formatBigDecimal10e3(int reps) {
        format(BD10E3, reps);
    }

    public void time_formatBigDecimal10e9(int reps) {
        format(BD10E9, reps);
    }

    public void time_formatBigDecimal10e100(int reps) {
        format(BD10E100, reps);
    }

    public void time_formatBigDecimal10e1000(int reps) {
        format(BD10E1000, reps);
    }

    public void time_formatPi(int reps) {
        format(Math.PI, reps);
    }

    public void time_formatE(int reps) {
        format(Math.E, reps);
    }

    public void time_formatUSD(int reps) {
        formatCurrencyUS(WHOLE_NUMBER, reps);
    }

    public void time_formatUsdWithCents(int reps) {
        formatCurrencyUS(TWO_DP_NUMBER, reps);
    }

    public void time_formatEur(int reps) {
        formatCurrencyFR(WHOLE_NUMBER, reps);
    }

    public void time_formatEurWithCents(int reps) {
        formatCurrencyFR(TWO_DP_NUMBER, reps);
    }

    public void time_formatAsExponent10e3(int reps) {
        format(EXP_PATTERN, BD10E3, reps);
    }

    public void time_formatAsExponent10e9(int reps) {
        format(EXP_PATTERN, BD10E9, reps);
    }

    public void time_formatAsExponent10e100(int reps) {
        format(EXP_PATTERN, BD10E100, reps);
    }

    public void time_formatAsExponent10e1000(int reps) {
        format(EXP_PATTERN, BD10E1000, reps);
    }

    public void time_formatToCharacterIterator10e3(int reps) {
        formatToCharacterIterator(BD10E3, reps);
    }

    public void time_formatToCharacterIterator10e9(int reps) {
        formatToCharacterIterator(BD10E9, reps);
    }

    public void time_formatToCharacterIterator10e100(int reps) {
        formatToCharacterIterator(BD10E100, reps);
    }

    public void time_formatToCharacterIterator10e1000(int reps) {
        formatToCharacterIterator(BD10E1000, reps);
    }
}

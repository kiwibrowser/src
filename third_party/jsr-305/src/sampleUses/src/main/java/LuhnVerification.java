public class LuhnVerification {

	static boolean checkNumber(String value) {
		int result = 0;
		boolean special = false;
		for (int i = value.length() - 1; i >= 0; i--) {
			int v = value.charAt(i) - '0';
			if (v < 0 || v > 9)
				return false;
			if (special) {
				v = v * 2;
				if (v > 9)
					v = v - 10 + 1;
			}
			result += v;
			special = !special;
		}
		System.out.println(result);
		return result % 10 == 0;
	}

	public static void main(String args[]) {
		System.out.println(checkNumber(""));
	}

}

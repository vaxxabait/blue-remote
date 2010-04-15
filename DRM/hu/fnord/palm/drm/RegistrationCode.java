package hu.fnord.palm.drm;

// javac RegistrationCode.java && java -classpath . RegistrationCode "HotSyncId"
public final class RegistrationCode {
    /* Application-specific constants. */
    public final static String APPNAME = "BlueRemote";
    public final static String PASSPHRASE = "k9ssGFsKL6oIauPLaX5t3xkNLTYMsWrP";
    public final static int KEY = 0x7A16892C;

    /* Digit codes.  */
    public final static String LINE0 = "0123456789ABCDEF";
    public final static String LINE1 = "HJKLMNPQRTUVWXYZ";
    
    public static String cleanup (String str) {
	StringBuffer r = new StringBuffer();
	for (int i = 0; i < str.length(); i++) {
	    char c = str.charAt (i);
	    
	    if (c == 0)
		break;
	    
	    if (c > 0xFF)	// Non-Latin-1
		continue;
	    if (c < 33 		// Low control characters and space
		|| c == 0x7F	// DEL
		|| (c >= 0x80 && c <= 0x9F) // High control characters
		|| (c >= 0xA0 && c <= 0xBF) // Fancy symbols
		|| c == 0xD7		    // Multiplication sign
		|| c == 0xF7)		    // Division sign
		continue;

	    // Replacements:
	    if (c == '`') c = '\'';
	    if (c == '_') c = '-';

	    // Convert to uppercase (ASCII only)
	    if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
	    
	    r.append (c);
	}
	if (r.length() > 32)
	    r.setLength (32);
	return r.toString();
    }

    private static int getValue (String id) throws Exception {
	/* Cleartext */
	String text = APPNAME + "_" + PASSPHRASE + "_"
	    + cleanup (id) + "_R";

	/* Produce an SHA1 hash of the cleartext.  */
	SHA1 sha1 = new SHA1();
	sha1.append (text.getBytes ("ISO8859-1"));
	byte[] digest = sha1.getDigest();

	/* Shorten 20-byte digest to a 32-bit value.  */
	int val = 0;
	for (int i = 0; i < 5; i++) {
	    int v = 0;
	    for (int j = 0; j < 4; j++) {
		v <<= 8;
		v |= ((int)digest[(i << 2) + j]) & 0xFF;
	    }
	    val ^= v;
	}
	return val;
    }

    private static byte getChecksum (int val) throws Exception {
	/* Produce an encoded checksum that is useful for detecting
	   HotSync name mismatches. */
	int key = val ^ KEY;
	byte checksum = (byte)
	    (key & 0xFF
	     ^ ((key >>> 8) & 0xFF)
	     ^ ((key >>> 16) & 0xFF)
	     ^ (((key >>> 24) & 0xFF)));
	return checksum;
    }
    
    
    public static String generate (String id) throws Exception {
	int val = getValue (id);
	byte checksum = getChecksum (val);

	/* Encode the checksum in the digits of the shortened digest.  */
	StringBuffer code = new StringBuffer();
	for (int i = 0; i < 8; i++) {
	    int digit = val & 0x0F;
	    if ((checksum & 0x01) == 1)
		code.append (LINE1.charAt (digit));
	    else
		code.append (LINE0.charAt (digit));
	    val >>>= 4;
	    checksum >>>= 1;
	}
	/* We want this to be big-endian.  */
	code.reverse();

	return code.toString();
    }

    public static void verify (String id, String code) throws Exception {
	code = cleanup (code);
	if (code.length() != 8)
	    throw new Exception ("Invalid registration code length");

	byte checksum = 0;
	int val = 0;
	for (int i = 0; i < code.length(); i++) {
	    char c = code.charAt (i);
	    if (c == 'O') c = '0';
	    if (c == 'I') c = '1';
	    if (c == 'G') c = '6';
	    if (c == 'S') c = '5';
	    int i0 = LINE0.indexOf (c);
	    int i1 = LINE1.indexOf (c);
	    int v;
	    if (i0 != -1) {
		v = i0;
		checksum <<= 1;
	    }
	    else if (i1 != -1) {
		v = i1;
		checksum <<= 1;
		checksum++;
	    }
	    else throw new Exception ("Invalid character '" + c + "'");

	    if (v > 15 || v < 0)
		throw new Exception ("Invalid digit value '" + c + "'");
	    
	    val <<= 4;
	    val |= v & 0x0F;
	}

	byte expectedChecksum = getChecksum (val);
	if (checksum != expectedChecksum)
	    throw new Exception ("Checksum mismatch");

	int expectedVal = getValue (id);
	if (val != expectedVal)
	    throw new Exception ("Value mismatch");
    }
    
    public static void main (String[] args) throws Exception {
	SHA1 hash = new SHA1();
	//System.out.println (hash.getASCIIDigest (args[0].getBytes ("ISO8859-1")));
	
	if (args.length == 1) {
	    String code = generate (args[0]);
	    System.out.println ("       HotSync ID: " + args[0]);
	    System.out.println ("Registration code: " + code);
	}
	else if (args.length == 2) {
	    verify (args[0], args[1]);
	    System.out.println ("Code seems OK");
	}
	else {
	    System.out.println ("Usage: RegistrationCode <HotsyncID>");
	    System.exit (1);
	}

    }
}

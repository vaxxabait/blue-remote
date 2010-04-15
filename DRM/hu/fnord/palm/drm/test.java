{
    {
	
//////////////////////////////////////////////////////////////////////
	result = RCgenerate (userID);
    }
    catch (Exception e) {
	throw new FunctionException (e.toString());
    }
    return result;
}


/* The SHA magic numbers.  */
private final int K1 = 0x5A827999;
private final int K2 = 0x6ED9EBA1;
private final int K3 = 0x8F1BBCDC;
private final int K4 = 0xCA62C1D6;

private long count;
private byte[] chunk = new byte[64];
private final byte[] padding = new byte[64];
private int[] digest = new int[5];

public void SHA1init() {
    count = 0;	
    padding[0] = (byte) 0x80;
    digest[0] = 0x67452301;
    digest[1] = 0xEFCDAB89;
    digest[2] = 0x98BADCFE;
    digest[3] = 0x10325476;
    digest[4] = 0xC3D2E1F0;
}

private int SHA1f1 (int x, int y, int z) {
    return (z ^ (x & (y ^ z))); // x ? y : z
}
		
private int SHA1f2 (int x, int y, int z) {
    return (x ^ y ^ z);	// XOR
}

private int SHA1f3 (int x, int y, int z) {
    return ((x & y) + (z & (x ^ y))); // majority
}

private int SHA1rol (int val, int d) {
    //return Integer.rotateLeft (val, d);
    return (val << d) | (val >>> (32 - d));
}

private void SHA1transform() throws Exception {
    int a, b, c, d, e, t, i, j;
    int[] w = new int[80];

    if ((count & 0x3F) != 0)
	throw new Exception ("Bad chunk size");

    int cnt = 0;
    for (i = 0; i < 16; i++) {
	w[i] = 0;
	for (j = 0; j < 4; j++) {
	    w[i] <<= 8;
	    w[i] |= chunk[cnt++] & 0xFF;
	}
    }
    for (i = 0; i < 64; i++)
	w[i+16] = SHA1rol (w[i+13] ^ w[i+8] ^ w[i+2] ^ w[i], 1);
	    
    a = digest[0];
    b = digest[1];
    c = digest[2];
    d = digest[3];
    e = digest[4];
	    
    for (i = 0; i < 20; i++) {
	t = SHA1f1 (b, c, d) + K1 + SHA1rol (a, 5) + e + w[i];
	e = d; d = c; c = SHA1rol (b, 30); b = a; a = t;
    }
	    
    for (; i < 40; i++) {
	t = SHA1f2 (b, c, d) + K2 + SHA1rol (a, 5) + e + w[i];
	e = d; d = c; c = SHA1rol (b, 30); b = a; a = t;
    }
	    
    for (; i < 60; i++) {
	t = SHA1f3 (b, c, d) + K3 + SHA1rol (a, 5) + e + w[i];
	e = d; d = c; c = SHA1rol (b, 30); b = a; a = t;
    }
	    
    for (; i < 80; i++) {
	t = SHA1f2 (b, c, d) + K4 + SHA1rol (a, 5) + e + w[i];
	e = d; d = c; c = SHA1rol (b, 30); b = a; a = t;
    }

    digest[0] += a;
    digest[1] += b;
    digest[2] += c;
    digest[3] += d;
    digest[4] += e;
}

public void SHA1append (byte[] text, int offset, int length)
    throws Exception {
    for (int i = offset; i < length; i++) {
	chunk[(int)((count++) & 0x3F)] = text[i];
	if ((count & 0x3F) == 0) {
	    SHA1transform();
	}
    }
}

public void SHA1append (byte[] text) throws Exception {
    SHA1append (text, 0, text.length);
}
	
public byte[] SHA1getDigest() throws Exception {
    // Add padding.
    int buflen = (int) (count & 0x3F);
    int padlen = (buflen < 56 ? 56 : 120) - buflen;
    long bits = count << 3;
    SHA1append (padding, 0, padlen);
    byte[] size = new byte[8];
    for (int i = 7; i >= 0; i--) {
	size[i] = (byte) (bits & 0xFF);
	bits >>>= 8;
    }
    SHA1append (size);

    if ((count & 0x3F) != 0)
	throw new Exception ("Bad padding: " + count);

    byte[] res = new byte[20];
    int j = 0;
    for (int i = 0; i < 5; i++) {
	res[j++] = (byte) (digest[i] >>> 24);
	res[j++] = (byte) (digest[i] >>> 16);
	res[j++] = (byte) (digest[i] >>> 8);
	res[j++] = (byte) (digest[i]);
    }
    return res;
}

public byte[] SHA1getDigest (byte[] msg) throws Exception {
    SHA1init();
    SHA1append (msg);
    return SHA1getDigest();
}

/* Application-specific constants. */
public final String APPNAME = "BlueRemote";
public final String PASSPHRASE = "k9ssGFsKL6oIauPLaX5t3xkNLTYMsWrP";
public final int KEY = 0x7A16892C;

/* Digit codes.  */
public final String LINE0 = "0123456789ABCDEF";
public final String LINE1 = "HJKLMNPQRTUVWXYZ";
    
public String RCcleanup (String str) {
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

private int RCgetValue (String id) throws Exception {
    /* Cleartext */
    String text = APPNAME + "_" + PASSPHRASE + "_"
	+ RCcleanup (id) + "_R";

    /* Produce an SHA1 hash of the cleartext.  */
    byte[] digest = SHA1getDigest (text.getBytes ("ISO8859-1"));

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

private byte RCgetChecksum (int val) throws Exception {
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
    
    
public String RCgenerate (String id) throws Exception {
    int val = RCgetValue (id);
    byte checksum = RCgetChecksum (val);

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

public void RCverify (String id, String code) throws Exception {
    code = RCcleanup (code);
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

    byte expectedChecksum = RCgetChecksum (val);
    if (checksum != expectedChecksum)
	throw new Exception ("Checksum mismatch");

    int expectedVal = RCgetValue (id);
    if (val != expectedVal)
	throw new Exception ("Value mismatch");
}


private String dummy ()  throws FunctionException {
    String result = null;
    try {
        result = "foobar";
//////////////////////////////////////////////////////////////////////
    }
}

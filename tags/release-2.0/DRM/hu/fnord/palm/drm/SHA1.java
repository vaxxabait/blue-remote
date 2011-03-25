package hu.fnord.palm.drm;

/* SHA1 implementation after Peter Gutmann's public domain code.  */
public final class SHA1 {
    /* The SHA magic numbers.  */
    private static final int K1 = 0x5A827999;
    private static final int K2 = 0x6ED9EBA1;
    private static final int K3 = 0x8F1BBCDC;
    private static final int K4 = 0xCA62C1D6;

    private long count;
    private byte[] chunk = new byte[64];
    static private final byte[] padding = new byte[64];
    private int[] digest = new int[5];

    static {
	padding[0] = (byte) 0x80;
    }
	
    public SHA1() {
	init();
    }

    public void init() {
	count = 0;
	digest[0] = 0x67452301;
	digest[1] = 0xEFCDAB89;
	digest[2] = 0x98BADCFE;
	digest[3] = 0x10325476;
	digest[4] = 0xC3D2E1F0;
    }

    private int f1 (int x, int y, int z) {
	return (z ^ (x & (y ^ z))); // x ? y : z
    }
		
    private int f2 (int x, int y, int z) {
	return (x ^ y ^ z);	// XOR
    }

    private int f3 (int x, int y, int z) {
	return ((x & y) + (z & (x ^ y))); // majority
    }

    private int rol (int val, int d) {
	//return Integer.rotateLeft (val, d);
	return (val << d) | (val >>> (32 - d));
    }

    private void transform() throws Exception {
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
	    w[i+16] = rol (w[i+13] ^ w[i+8] ^ w[i+2] ^ w[i], 1);
	    
	a = digest[0];
	b = digest[1];
	c = digest[2];
	d = digest[3];
	e = digest[4];
	    
	for (i = 0; i < 20; i++) {
	    t = f1 (b, c, d) + K1 + rol (a, 5) + e + w[i];
	    e = d; d = c; c = rol (b, 30); b = a; a = t;
	}
	    
	for (; i < 40; i++) {
	    t = f2 (b, c, d) + K2 + rol (a, 5) + e + w[i];
	    e = d; d = c; c = rol (b, 30); b = a; a = t;
	}
	    
	for (; i < 60; i++) {
	    t = f3 (b, c, d) + K3 + rol (a, 5) + e + w[i];
	    e = d; d = c; c = rol (b, 30); b = a; a = t;
	}
	    
	for (; i < 80; i++) {
	    t = f2 (b, c, d) + K4 + rol (a, 5) + e + w[i];
	    e = d; d = c; c = rol (b, 30); b = a; a = t;
	}

	digest[0] += a;
	digest[1] += b;
	digest[2] += c;
	digest[3] += d;
	digest[4] += e;
    }

    public void append (byte[] text, int offset, int length)
	throws Exception {
	for (int i = offset; i < length; i++) {
	    chunk[(int)((count++) & 0x3F)] = text[i];
	    if ((count & 0x3F) == 0) {
		transform();
	    }
	}
    }

    public void append (byte[] text) throws Exception {
	append (text, 0, text.length);
    }
	
    public byte[] getDigest() throws Exception {
	// Add padding.
	int buflen = (int) (count & 0x3F);
	int padlen = (buflen < 56 ? 56 : 120) - buflen;
	long bits = count << 3;
	append (padding, 0, padlen);
	byte[] size = new byte[8];
	for (int i = 7; i >= 0; i--) {
	    size[i] = (byte) (bits & 0xFF);
	    bits >>>= 8;
	}
	append (size);

	if ((count & 0x3F) != 0)
	    throw new Exception ("Bad padding: " + count);

	byte[] result = new byte[20];
	int j = 0;
	for (int i = 0; i < 5; i++) {
	    result[j++] = (byte) (digest[i] >>> 24);
	    result[j++] = (byte) (digest[i] >>> 16);
	    result[j++] = (byte) (digest[i] >>> 8);
	    result[j++] = (byte) (digest[i]);
	}
	return result;
    }

    public byte[] getDigest (byte[] msg) throws Exception {
	init();
	append (msg);
	return getDigest();
    }

    public byte[] getDigest (String msg) throws Exception {
	return getDigest (msg.getBytes ("ISO8859-1"));
    }
	
    public String toASCII (byte[] digest) throws Exception {
	StringBuffer b = new StringBuffer();
	for (int i = 0; i < 5; i++) {
	    int val = 0;
	    for (int j = 0; j < 4; j++) {
		val <<= 8;
		val |= (digest[(i << 2) + j]) & 0xFF;
	    }
	    b.append (Integer.toHexString (val));
	}
	return b.toString();
    }

    public String getASCIIDigest() throws Exception {
	return toASCII (getDigest());
    }
}

package jCode;

/*
 * @(#)jXXXX.java 
 *
 * Copyright 1999-2003, PalmGear.com.
 *                      Aaron Ardiri (designer/developer)
 *
 * The information and source code presented in this Java source file
 * are proprietary. This source code shall only be used solely for use 
 * within the RealTime Fulfillment module of PalmGear.com Redistribution
 * for other purposes is not permitted. 
 *
 * - http://www.palmgear.com/ 
 * 
 * TEMPLATE HISTORY:
 * @version 1.00 (9 November 1999) - creation
 */

import com.palmgear.jCode.Function;
import com.palmgear.jCode.FunctionException;

/**
 * The RealTime Fulfillment registration code generator for the following
 * product for sale online at "<tt>PalmGearHQ</tt>": <p>
 * 
 * - BlueRemote Bluetooth Remote Control <p>
 *
 * @author <a href="mailto:palmgear@lorentey.hu">palmgear@lorentey.hu</a>
 * @version (1 June 2000)
 *
 * @see Function
 */
public class j124035 extends Function
{
  /**
   * Construct a jXXXX instance.
   */
  public j124035()
  {
    super();
  }

  /**
   * Get the Product Name.
   *
   * @return the Product name.
   */
  public String getProductName() 
  {
    return "BlueRemote Bluetooth Remote Control";
  }
 
  /**
   * The implementation of the "custom" code generator using a userID.
   *
   * @param userID the HotSync username of the Palm Computing user.
   * @return A custom generated code for the HotSync username.
   * @exception FunctionException An error occured while generating the code.
   */
  public String codeFunction(String userID)
    throws FunctionException
  {
    String result = null;

    // attempt the code generation
    try {
	if (false) {
	    int unlockCode = 0;
	    for (int i=0; i < userID.length(); i++) {
		unlockCode = unlockCode + (byte)userID.charAt(i);
	    }
	    result = Integer.toString(unlockCode & 0xFFFF);
	}
//////////////////////////////////////////////////////////////////////////

    /* SHA1 implementation after Peter Gutmann's public domain code.  */
    class SHA1 {
	/* The SHA magic numbers.  */
	private static final int K1 = 0x5A827999;
	private static final int K2 = 0x6ED9EBA1;
	private static final int K3 = 0x8F1BBCDC;
	private static final int K4 = 0xCA62C1D6;

	private long count;
	private byte[] chunk = new byte[64];
	private final byte[] padding = new byte[64];
	private int[] digest = new int[5];

	public void init() {
	    count = 0;	
	    padding[0] = (byte) 0x80;
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

	public byte[] getDigest (byte[] msg) throws Exception {
	    init();
	    append (msg);
	    return getDigest();
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

	public String getASCIIDigest (byte[] msg) throws Exception {
	    return toASCII (getDigest (msg));
	}
    }
	
    class RegistrationCode {
	/* Application-specific constants. */
	public final String APPNAME = "BlueRemote";
	public final String PASSPHRASE = "k9ssGFsKL6oIauPLaX5t3xkNLTYMsWrP";
	public final int KEY = 0x7A16892C;

	/* Digit codes.  */
	public final String LINE0 = "0123456789ABCDEF";
	public final String LINE1 = "HJKLMNPQRTUVWXYZ";
    
	public String cleanup (String str) {
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

	private int getValue (String id) throws Exception {
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

	private byte getChecksum (int val) throws Exception {
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
    
    
	public String generate (String id) throws Exception {
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
    }

    RegistrationCode rc = new RegistrationCode();
    result = rc.generate (userID);
    rc.verify (userID, result);
    
//////////////////////////////////////////////////////////////////////////

    }

    // an error occurred.. terminate
    catch (Exception e) {
      throw
        new FunctionException(e.toString());
    }

    return result;
  }
}

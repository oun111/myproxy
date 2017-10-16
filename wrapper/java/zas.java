import java.awt.Robot;
//import java.lang

public class zas {

	static{
    /*
     * TODO: set correct path of the cwpr library
     */
		//System.load("/mnt/sda7/work/zas/wrapper/libcwpr.0.0.1.so");
		System.load("/mnt/sda5/zyw/work/zas/wrapper/libcwpr.0.0.1.so");
	}

  /*
   * jni generated methods
   */
	public native int login(String host, int port, String usr, String pwd, String db);

	public native int prepare(String stmt);

	public native int insertStr(String val);

	public native int insertBigint(int val);

	public native int insertLong(long val);

	public native int insertInt(int val);

	public native int insertUnsigned(int val);

	public native int insertShort(short val);

	public native int insertUnsignedShort(short val);

	public native int insertDouble(double val);

	public native int insertLongDouble(double val);

	public native int insertFloat(float val);


	public native boolean isEof();

	public native String fetchStr();

	public native int fetchInt();

	public native int fetchBigint();

	public native long fetchLong();

	public native int fetchUnsigned();

	public native short fetchShort();

	public native short fetchUnsignedShort();

	public native double fetchDouble();

	public native double fetchLongDouble();

	public native float fetchFloat();

/*
	public static void main(String[] args) throws Exception {
    zas mz = new zas() ;

    mz.login("localhost",3306,"root","123","");

    mz.prepare("select id,name,price,size from test_db.test_tbl where id<:f1<int>");

    mz.insertInt(10);

    while (!mz.isEof()) {
      int id = 0;
      String name = "" ;
      double price = 0;
      long size = 0;

      id = mz.fetchInt();
      name = mz.fetchStr();
      price = mz.fetchDouble();
      size = mz.fetchLong();
      System.out.println(id + ": name: " + name + ", point: " + 
        price + ", size: " + size + "\n");
    }

    System.out.println("ok!\n");

	}
*/

}


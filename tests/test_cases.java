
import java.awt.Robot;

/*
 * TODO: set correct path of zas.class before compiling: 
 *
 *  export CLASSPATH=$CLASSPATH:/mnt/sda5/zyw/work/zas/wrapper/java
 */

public class test_cases {

	public static void main(String[] args) throws Exception {

    zas mz = (zas)Class.forName("zas").newInstance();

    mz.login("localhost",3306,"root","123","");

    mz.prepare("select id,name,price,size from test_db.test_tbl where id<:f1<int>");

    while (true) {

      mz.insertInt(10);

      while (!mz.isEof()) {
        Integer id = 0;
        String name = "" ;
        Double price = 0.0;
        Long size = 0L;

        id = mz.fetchInt();
        name = mz.fetchStr();
        price = mz.fetchDouble();
        size = mz.fetchLong();
        System.out.println(id + ": name: " + name + ", point: " + 
          price + ", size: " + size + "\n");
      }

      //System.out.println("ok!\n");

    }

  }
}


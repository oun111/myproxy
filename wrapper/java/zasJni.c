
#include <jni.h>
#include "cwpr.h"

/*
 * METHOD PROTOTYPES IN THIS FILE SHOULD BE IDENTICAL 
 *  WTIH THOSE IN zasJni.h
 *
 */

extern "C" {

JNIEXPORT jint JNICALL Java_zas_login
  (JNIEnv *env, jobject, jstring host, jint port, 
   jstring usr, jstring pwd, jstring db)
{
  int ret= 0;
  char connstr[512] = "";
  char* ph = (char*)env->GetStringUTFChars(host,NULL);
  char* pu = (char*)env->GetStringUTFChars(usr,NULL);
  char* pp = (char*)env->GetStringUTFChars(pwd,NULL);
  char* pd = (char*)env->GetStringUTFChars(db,NULL);

  sprintf(connstr,"server=%s;port=%d;usr=%s;pwd=%s;"
    "unix_socket=/tmp/mysql.sock;dbname=%s;",
    ph,port,pu,pp,pd);

  ret = rlogon(connstr);

  env->ReleaseStringUTFChars(host,ph);
  env->ReleaseStringUTFChars(usr,pu);
  env->ReleaseStringUTFChars(pwd,pp);
  env->ReleaseStringUTFChars(db,pd);

  return ret;
}

JNIEXPORT jint JNICALL Java_zas_prepare
  (JNIEnv *env, jobject , jstring stmt)
{
  char* ps = (char*)env->GetStringUTFChars(stmt,NULL);
  int ret = prepare_stmt(ps);

  if (ret) {
    return -1;
  }

  env->ReleaseStringUTFChars(stmt,ps);

  //printd("prepare ok\n");

  return ret;
}

/*
 * insertion methods
 */
JNIEXPORT jint JNICALL Java_zas_insertStr
  (JNIEnv *env, jobject, jstring val)
{
  char* pv = (char*)env->GetStringUTFChars(val,NULL);
  int ret = prepare_stmt(pv);

  env->ReleaseStringUTFChars(val,pv);

  return ret;
}

JNIEXPORT jint JNICALL Java_zas_insertBigint
  (JNIEnv *env, jobject, jint val)
{
  return insert_int(val);
}

JNIEXPORT jint JNICALL Java_zas_insertLong
  (JNIEnv *, jobject, jlong val)
{
  return insert_long(val);
}

JNIEXPORT jint JNICALL Java_zas_insertInt
  (JNIEnv *, jobject, jint val)
{
  return insert_int(val);
}

JNIEXPORT jint JNICALL Java_zas_insertUnsigned
  (JNIEnv *, jobject, jint val)
{
  return insert_int(val);
}

JNIEXPORT jint JNICALL Java_zas_insertShort
  (JNIEnv *, jobject, jshort val)
{
  return insert_short(val);
}

JNIEXPORT jint JNICALL Java_zas_insertUnsignedShort
  (JNIEnv *, jobject, jshort val)
{
  return insert_unsigned_short(val);
}

JNIEXPORT jint JNICALL Java_zas_insertDouble
  (JNIEnv *, jobject, jdouble val)
{
  return insert_double(val);
}

JNIEXPORT jint JNICALL Java_zas_insertLongDouble
  (JNIEnv *, jobject, jdouble val)
{
  return insert_long_double(val);
}

JNIEXPORT jint JNICALL Java_zas_insertFloat
  (JNIEnv *, jobject, jfloat val)
{
  return insert_float(val);
}

/*
 * fetching methods
 */
JNIEXPORT jboolean JNICALL Java_zas_isEof
  (JNIEnv *, jobject)
{
  return is_eof();
}

/* refer from internet */
static jstring charTojstring(JNIEnv* env, const char* pat) {
	//定义java String类 strClass
	jclass strClass = (env)->FindClass("Ljava/lang/String;");
	//获取String(byte[],String)的构造器,用于将本地byte[]数组转换为一个新String
	jmethodID ctorID = (env)->GetMethodID(strClass, "<init>", "([BLjava/lang/String;)V");
	//建立byte数组
	jbyteArray bytes = (env)->NewByteArray(strlen(pat));

	//将char* 转换为byte数组
	(env)->SetByteArrayRegion(bytes, 0, strlen(pat), (jbyte*) pat);
	// 设置String, 保存语言类型,用于byte数组转换至String时的参数
	jstring encoding = (env)->NewStringUTF("utf8");
	//将byte数组转换为java String,并输出
	jstring ret = (jstring) (env)->NewObject(strClass, ctorID, bytes, encoding);

  env->DeleteLocalRef(bytes);
  env->DeleteLocalRef(encoding);
  env->DeleteLocalRef(strClass); 

  return ret ;
}


JNIEXPORT jstring JNICALL Java_zas_fetchStr
  (JNIEnv *env, jobject)
{
  char pval[LONG_DATABUF_LEN] = "";
  int ret = fetch_str(pval);

  /*
   * FIXME: memory leaks in this function
   */
  jstring val = charTojstring(env,pval);

  (void)ret;
  //env->DeleteLocalRef(val); 

  return val;
}

JNIEXPORT jint JNICALL Java_zas_fetchInt
  (JNIEnv *, jobject)
{
  int val = 0;

  fetch_int(val);
  return val;
}

JNIEXPORT jint JNICALL Java_zas_fetchBigint
  (JNIEnv *, jobject)
{
  long long val = 0;

  fetch_big_int(val);
  return val;
}

JNIEXPORT jlong JNICALL Java_zas_fetchLong
  (JNIEnv *, jobject)
{
  long val = 0;

  fetch_long(val);
  return val;
}

JNIEXPORT jint JNICALL Java_zas_fetchUnsigned
  (JNIEnv *, jobject)
{
  unsigned val = 0;

  fetch_unsigned(val);
  return val;
}

JNIEXPORT jshort JNICALL Java_zas_fetchShort
  (JNIEnv *, jobject)
{
  short val = 0;

  fetch_short(val);
  return val;
}

JNIEXPORT jshort JNICALL Java_zas_fetchUnsignedShort
  (JNIEnv *, jobject)
{
  unsigned short val = 0;

  fetch_unsigned_short(val);
  return val;
}

JNIEXPORT jdouble JNICALL Java_zas_fetchDouble
  (JNIEnv *, jobject)
{
  double val = 0;

  fetch_double(val);
  return val;
}

JNIEXPORT jdouble JNICALL Java_zas_fetchLongDouble
  (JNIEnv *, jobject)
{
  long double val = 0;

  fetch_long_double(val);
  return val;
}

JNIEXPORT jfloat JNICALL Java_zas_fetchFloat
  (JNIEnv *, jobject)
{
  float val = 0;

  fetch_float(val);
  return val;
}
}


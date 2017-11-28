
#ifndef __THREAD_HELPER_H__
#define __THREAD_HELPER_H__

#include <pthread.h>
#include <stdio.h>


class thread_info {
public:
  using PROC_PTR= void(*)(size_t,char*);
  /* the target class  */
  size_t obj ;
  /* the thread proc */
  PROC_PTR func ;
  /* the proc argument */
  char *arg ;
  /* lock & conditions */
  pthread_mutex_t *mtx ;
  //pthread_cond_t *cond ;
} ;

namespace th_context {
  extern "C" {
    void* thread_proc(void*);
  }
} ;

template<typename FUNC_TYPE,typename CLASS_TYPE>
class thread_helper {

protected:
  /* the thread info */
  thread_info m_ti ;
  /* thread mutex */
  pthread_mutex_t m_mtx ;
  /* thread wait condition */
  pthread_cond_t m_cond ;
  /* thread handle */
  pthread_t m_thd ;

public:
  thread_helper(void) 
  {
    /* lock and wait conditions */
    pthread_mutex_init(&m_mtx,0);
    pthread_cond_init(&m_cond,0);
  }
  ~thread_helper(void) 
  {
    pthread_cancel(m_thd);
    pthread_mutex_destroy(&m_mtx);
    pthread_cond_destroy(&m_cond);
  }

public:

  int bind(FUNC_TYPE f,CLASS_TYPE *c,char *arg)
  {
    /* init the thread info */
    m_ti.func = *((thread_info::PROC_PTR*)&f);
    m_ti.obj  = (size_t)c;
    m_ti.arg  = arg ;
    m_ti.mtx  = &m_mtx ;
    return 0;
  }
  int start(void) 
  {
    /* boost the thread itself */
    if (pthread_create(&m_thd, 0, 
       th_context::thread_proc,
       (void*)&m_ti)) {
      return -1;
    }
    return 0;
  }
  void wait(void)
  {
    pthread_cond_wait(&m_cond,&m_mtx);
  }
  void signal(void)
  {
    pthread_cond_signal(&m_cond);
  }
};

#endif /* __THREAD_HELPER_H__*/


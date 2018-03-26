
#include <stdint.h>
#include "thread_helper.h"


namespace th_context {

  extern "C" {

    void* thread_proc(void *arg) 
    {
      thread_info *pi = (thread_info*)arg;

      pthread_mutex_lock(pi->mtx);
      /* the the actual thread function */
      pi->func(pi->obj,pi->arg);
      pthread_mutex_unlock(pi->mtx);
      return NULL ;
    }

  }

} ;

/* 
 * FIXME: the g++ compiler dosen't support 'export' 
 *  keyword for template class implementations in
 *  'cpp' files
 */

#if 0
template <typename FUNC_TYPE>
thread_helper<FUNC_TYPE>::thread_helper(void) 
{
  /* lock and wait conditions */
  pthread_mutex_init(&m_mtx,0);
  pthread_cond_init(&m_cond,0);
}

template <typename FUNC_TYPE>
thread_helper<FUNC_TYPE>::~thread_helper(void) 
{
  pthread_cancel(m_thd);
  pthread_mutex_destroy(&m_mtx);
  pthread_cond_destroy(&m_cond);
}

template <typename FUNC_TYPE>
int thread_helper<FUNC_TYPE>::bind(FUNC_TYPE f, char *arg)
{
  /* init the thread info */
  m_ti.func = *((thread_info::PROC_PTR*)&f);
  m_ti.arg  = arg ;
  m_ti.mtx  = &m_mtx ;
  return 0;
}

template <typename FUNC_TYPE>
int thread_helper<FUNC_TYPE>::start(void)
{
  /* boost the thread itself */
  if (pthread_create(&m_thd, 0, thread_proc,
     (void*)&m_ti)) {
    return -1;
  }
  return 0;
}

template <typename FUNC_TYPE>
void thread_helper<FUNC_TYPE>::wait(void)
{
  pthread_cond_wait(&m_cond,&m_mtx);
}

template <typename FUNC_TYPE>
void thread_helper<FUNC_TYPE>::signal(void)
{
  pthread_cond_signal(&m_cond);
}
#endif



#include <iostream>
#include <algorithm>
#include "epi_list.h"

bool vcomp(VP_TYPE &lp, VP_TYPE &rp) 
{
  return lp.second < rp.second; 
}

epi_list::epi_list()
{
  pthread_rwlock_init(&m_lk,0); 
}

epi_list::~epi_list()
{
  clear();

  pthread_rwlock_destroy(&m_lk);
}

void epi_list::clear(void)
{
  try_write_lock();

  m_list.clear();

  m_vec.clear();
}

int epi_list::get(sock_toolkit *k, ITRM &itr) 
{
  itr = m_list.find(k);
  if (itr==m_list.end()) {
    return -1;
  }
  return 0;
}

int epi_list::insert(sock_toolkit *k, int v) 
{
  try_write_lock();

  m_list.emplace(k,v);

  m_vec.push_back(VP_TYPE(k,v));

  sort(m_vec.begin(),m_vec.end(),vcomp);

  return 0;
}

int epi_list::get_idle(sock_toolkit* &k)
{
  int v= 0 ;
  ITRV itrv ;
  ITRM itrm ;
  
  {
    try_write_lock();

    /* get 1st key & value from ordered vector */
    itrv = m_vec.begin();
    if (itrv==m_vec.end()) {
      return -1;
    }

    k = (*itrv).first;
    v = ++((*itrv).second) ;

    /* update value */
    (*itrv).second = v;
    /* re-order */
    sort(m_vec.begin(),m_vec.end(),vcomp);
  }

  {
    try_write_lock();

    /* get itr from map by key */
    if (get(k,itrm)) {
      return -1;
    }

    /* update value in map */
    (*itrm).second = v;
  }
  
  return 0;
}

int epi_list::return_back(sock_toolkit* &k)
{
  int v= 0 ;
  ITRM itrm ;

  {
    try_write_lock();

    if (get(k,itrm)) {
      return -1;
    }

    auto &pv = (*itrm).second ;

    v  = pv ;
    pv = pv>0?(pv-1):0 ;
  }

  {
    try_write_lock();

    for (auto &i:m_vec) {
      if (i==VP_TYPE(k,v)) {
        auto &pv = i.second ;

        pv = pv>0?(pv-1):0;

        /* re-order */
        sort(m_vec.begin(),m_vec.end(),vcomp);

        break ;
      }
    }
  }

  return 0;
}

int epi_list::new_ep(sock_toolkit *k)
{
  return insert(k,0);
}

void epi_list::debug(void)
{
  std::cout << "items in map: \n" ;

  for (auto i:m_list) {
    std::cout << i.first << " " << i.second << "\n";
  }

  std::cout << "items in vec: \n" ;

  for (auto i:m_vec) {
    std::cout << i.first << " " << i.second << "\n";
  }
}


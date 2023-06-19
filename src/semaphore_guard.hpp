#ifndef SEMAPHORE_GUARD_HPP
#define SEMAPHORE_GUARD_HPP

#include <semaphore>

template<typename Sem>
class SemaphoreGuard
{
public:
  explicit SemaphoreGuard(Sem& sem) :
    m_sem(sem)
  {
    m_sem.acquire();
  }

  ~SemaphoreGuard() {
    m_sem.release();
  }

private:
  Sem& m_sem;

public:
  SemaphoreGuard(SemaphoreGuard const&) = delete;
  SemaphoreGuard& operator=(SemaphoreGuard const&) = delete;
};

#endif

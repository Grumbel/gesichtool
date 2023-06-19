// gesichtool - Face Extraction Tool
// Copyright (C) 2023 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_GESICHTOOL_SEMAPHORE_GUARD_HPP
#define HEADER_GESICHTOOL_SEMAPHORE_GUARD_HPP

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

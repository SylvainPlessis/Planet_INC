//-----------------------------------------------------------------------bl-
//--------------------------------------------------------------------------
//
// Planet - An atmospheric code for planetary bodies, adapted to Titan
//
// Copyright (C) 2013 The PECOS Development Team
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the Version 2.1 GNU Lesser General
// Public License as published by the Free Software Foundation.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc. 51 Franklin Street, Fifth Floor,
// Boston, MA  02110-1301  USA
//
//-----------------------------------------------------------------------el-

#ifndef PLANET_DIRW_PDF_H
#define PLANET_DIRW_PDF_H

//Antioch
#include "antioch/antioch_asserts.h"

//Planet
#include "planet/base_pdf.h"

//C++
#include <vector>

namespace Planet
{
  template <typename CoeffType>
  class DirWPdf: public BasePdf<CoeffType>
  {
      private:
        std::vector<CoeffType> _v;
        std::vector<CoeffType> _dv;

      public:
        DirWPdf();

        template <typename StateType>
        DirWPdf(const DirWPdf<StateType> & rhs);

        DirWPdf(const std::vector<CoeffType> &v, const std::vector<CoeffType> &dv);
        ~DirWPdf();

        template <typename StateType>
        void set_parameters(const std::vector<StateType> &pars);

        template <typename StateType>
        void get_parameters(std::vector<StateType> &pars) const;

        template <typename StateType>
        void set_v(const std::vector<StateType> &v);

        template <typename StateType>
        void set_dv(const std::vector<StateType> &dv);

        const std::vector<CoeffType> v()  const;

        const std::vector<CoeffType> dv() const;

        const CoeffType value(unsigned int ip = 0) const;

        void print(std::ostream &out = std::cout)  const;
  };

  template <typename CoeffType>
  inline
  DirWPdf<CoeffType>::DirWPdf():
      BasePdf<CoeffType>(PDFName::DirW)
  {
     return;
  }

  template <typename CoeffType>
  template <typename StateType>
  inline
  DirWPdf<CoeffType>::DirWPdf(const DirWPdf<StateType> & rhs):
      BasePdf<CoeffType>(PDFName::DirW)
  {
     _v.resize(rhs.v().size(),0.L);
     _dv.resize(rhs.v().size(),0.L);
     for(unsigned int i = 0; i < rhs.v().size(); i++)
     {
        _v[i]  = rhs.v()[i];
        _dv[i] = rhs.dv()[i];
     }

     return;
  }

  template <typename CoeffType>
  inline
  DirWPdf<CoeffType>::DirWPdf(const std::vector<CoeffType> &v, const std::vector<CoeffType> &dv):
      BasePdf<CoeffType>(PDFName::DirW),
      _v(v),
      _dv(dv)
  {
     return;
  }

  template <typename CoeffType>
  inline
  DirWPdf<CoeffType>::~DirWPdf()
  {
     return;
  }

  template <typename CoeffType>
  template <typename StateType>
  inline
  void DirWPdf<CoeffType>::set_v(const std::vector<StateType> &v)
  {
     _v.resize(v.size(),0.L);
     for(unsigned int i = 0; i < v.size(); i++)
     {
       _v[i] = v[i];
     }
  }

  template <typename CoeffType>
  template <typename StateType>
  inline
  void DirWPdf<CoeffType>::set_dv(const std::vector<StateType> &dv)
  {
     _dv.resize(dv.size(),0.L);
     for(unsigned int i = 0; i < dv.size(); i++)
     {
       _dv[i] = dv[i];
     }
  }

  template <typename CoeffType>
  inline
  const std::vector<CoeffType> DirWPdf<CoeffType>::v() const
  {
      return _v;
  }

  template <typename CoeffType>
  inline
  const std::vector<CoeffType> DirWPdf<CoeffType>::dv() const
  {
      return _dv;
  }

  template <typename CoeffType>
  inline
  const CoeffType DirWPdf<CoeffType>::value(unsigned int ip) const
  {
      return _v[ip];
  }

  template <typename CoeffType>
  template <typename StateType>
  inline
  void DirWPdf<CoeffType>::set_parameters(const std::vector<StateType> &pars)
  {
      antioch_assert_equal_to(pars.size()%2,0);

     _v.resize(pars.size()/2,0.L);
     _dv.resize(pars.size()/2,0.L);
     for(unsigned int i = 0; i < _v.size(); i++)
     {
       _v[i]  = pars[i];
       _dv[i] = pars[i + _v.size()];
     }
  }

  template <typename CoeffType>
  template <typename StateType>
  inline
  void DirWPdf<CoeffType>::get_parameters(std::vector<StateType> &pars) const
  {
     pars.resize(_v.size() * 2,-1.L);
     for(unsigned int i = 0; i < _v.size(); i++)
     {
       pars[i] = _v[i];
       pars[i + _v.size()] = _dv[i];
     }
  }

  template <typename CoeffType>
  inline
  void DirWPdf<CoeffType>::print(std::ostream &out)  const
  {
     out << "DirW";
     if(!_v.empty())
     {
       out << "(" ;
       out << _v[0];

       for(unsigned int ibr = 1; ibr < _v.size(); ibr++)
       {
          out << "," << _v[ibr];
       }
       out << "; " << _dv[0];
       for(unsigned int ibr = 1; ibr < _dv.size(); ibr++)
       {
          out << "," << _dv[ibr];
       }

       out << ")";
     }
  }
}

#endif

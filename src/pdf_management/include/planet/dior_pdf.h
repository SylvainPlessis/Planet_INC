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

#ifndef PLANET_DIOR_PDF_H
#define PLANET_DIOR_PDF_H

//Antioch
#include "antioch/antioch_asserts.h"

//Planet
#include "planet/base_pdf.h"

//C++
#include <vector>

namespace Planet
{
  template <typename CoeffType>
  class DiOrPdf: public BasePdf<CoeffType>
  {
      private:
        std::vector<unsigned int> _n;

      public:
        DiOrPdf();
        DiOrPdf(const std::vector<unsigned int> &n);
        ~DiOrPdf();

        template <typename StateType>
        void set_parameters(const std::vector<StateType> &pars);

        template <typename StateType>
        void get_parameters(std::vector<StateType> &pars) const;

        void set_n(std::vector<unsigned int> n);

        std::vector<unsigned int> n() const;

        const CoeffType value(unsigned int ip = 0) const;

        void print(std::ostream &out = std::cout)  const;
  };

  template <typename CoeffType>
  inline
  DiOrPdf<CoeffType>::DiOrPdf():
      BasePdf<CoeffType>(PDFName::DiOr)
  {
     return;
  }

  template <typename CoeffType>
  inline
  DiOrPdf<CoeffType>::DiOrPdf(const std::vector<unsigned int> &n):
      BasePdf<CoeffType>(PDFName::DiOr),
      _n(n)
  {
     return;
  }

  template <typename CoeffType>
  inline
  DiOrPdf<CoeffType>::~DiOrPdf()
  {
     return;
  }

  template <typename CoeffType>
  inline
  void DiOrPdf<CoeffType>::set_n(std::vector<unsigned int> n)
  {
     _n.resize(n.size(),0.L);
     for(unsigned int i = 0; i < n.size(); i++)
     {
       _n[i] = n[i];
     }
  }

  template <typename CoeffType>
  inline
  std::vector<unsigned int> DiOrPdf<CoeffType>::n() const
  {
      return _n;
  }

  template <typename CoeffType>
  inline
  const CoeffType DiOrPdf<CoeffType>::value(unsigned int ip) const
  {
      return CoeffType(2.) * (CoeffType(_n.size()) + CoeffType(1.) - CoeffType(_n[ip])) /
                             (CoeffType(_n.size()) * CoeffType(_n.size() + 1));
  }

  template <typename CoeffType>
  template <typename StateType>
  inline
  void DiOrPdf<CoeffType>::set_parameters(const std::vector<StateType> &pars)
  {

     _n.resize(pars.size(),0);
     for(unsigned int i = 0; i < pars.size(); i++)
     {
       _n[i] = (unsigned int)pars[i];
     }
  }

  template <typename CoeffType>
  template <typename StateType>
  inline
  void DiOrPdf<CoeffType>::get_parameters(std::vector<StateType> &pars) const
  {
     pars.resize(_n.size(),0.L);
     for(unsigned int i = 0; i < _n.size(); i++)
     {
       pars[i] = (StateType)_n[i];
     }
  }

  template <typename CoeffType>
  inline
  void DiOrPdf<CoeffType>::print(std::ostream &out)  const
  {
     out << "DiOr("
         << _n[0];

     for(unsigned int ibr = 1; ibr < _n.size(); ibr++)
     {
        out << "," << _n[ibr];
     }
     out << ")";
  }
}

#endif

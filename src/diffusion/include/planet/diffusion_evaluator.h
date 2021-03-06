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

#ifndef PLANET_DIFFUSION_EVALUATOR_H
#define PLANET_DIFFUSION_EVALUATOR_H

//Antioch
#include "antioch/antioch_asserts.h"
#include "antioch/chemical_mixture.h"

//Planet
#include "planet/molecular_diffusion_evaluator.h"
#include "planet/eddy_diffusion_evaluator.h"

//C++
#include <string>

namespace Planet{

  /*!\class DiffusionEvaluator
 * Stores all kind of diffusions
 *
 */
  template <typename CoeffType, typename VectorCoeffType, typename MatrixCoeffType>
  class DiffusionEvaluator
  {
      private:
       DiffusionEvaluator() {antioch_error();return;}

//dependencies
       const MolecularDiffusionEvaluator<CoeffType,VectorCoeffType,MatrixCoeffType> & _molecular_diffusion;
       const EddyDiffusionEvaluator<CoeffType,VectorCoeffType,MatrixCoeffType>      & _eddy_diffusion;
       const AtmosphericMixture<CoeffType,VectorCoeffType,MatrixCoeffType>          & _mixture;
       const AtmosphericTemperature<CoeffType,VectorCoeffType>                      & _temperature;

      public:
       //!
       DiffusionEvaluator(const MolecularDiffusionEvaluator<CoeffType,VectorCoeffType,MatrixCoeffType> &mol_diff,
                          const EddyDiffusionEvaluator<CoeffType,VectorCoeffType,MatrixCoeffType>      &eddy_diff,
                          const AtmosphericMixture<CoeffType,VectorCoeffType,MatrixCoeffType>          &mix,
                          const AtmosphericTemperature<CoeffType,VectorCoeffType>                      &temp);

        //!
       ~DiffusionEvaluator();

       //!
       template<typename StateType, typename VectorStateType>
       void diffusion(const VectorStateType &molar_concentrations,
                      const StateType &z, VectorStateType &omega_a, 
                                          VectorStateType &omega_b) const;

       //!
       template<typename StateType, typename VectorStateType, typename MatrixStateType>
       void diffusion_and_derivs(const VectorStateType &molar_concentrations,
                                 const StateType &z, 
                                 VectorStateType &omegas_A_term,
                                 VectorStateType &omegas_B_term,
                                 MatrixStateType &domegas_dn_i_A_TERM,
                                 MatrixStateType &domegas_dn_i_B_TERM) const;

  };

  template <typename CoeffType, typename VectorCoeffType, typename MatrixCoeffType>
  inline
  DiffusionEvaluator<CoeffType, VectorCoeffType,MatrixCoeffType>::DiffusionEvaluator(
                          const MolecularDiffusionEvaluator<CoeffType,VectorCoeffType,MatrixCoeffType> &mol_diff,
                          const EddyDiffusionEvaluator<CoeffType,VectorCoeffType,MatrixCoeffType>      &eddy_diff,
                          const AtmosphericMixture<CoeffType,VectorCoeffType,MatrixCoeffType>          &mix,
                          const AtmosphericTemperature<CoeffType,VectorCoeffType>                      &temp):
    _molecular_diffusion(mol_diff),
    _eddy_diffusion(eddy_diff),
    _mixture(mix),
    _temperature(temp)
  {
     return;
  }

  template <typename CoeffType, typename VectorCoeffType, typename MatrixCoeffType>
  inline
  DiffusionEvaluator<CoeffType, VectorCoeffType,MatrixCoeffType>::~DiffusionEvaluator()
  {
     return;
  }

  template <typename CoeffType, typename VectorCoeffType, typename MatrixCoeffType>
  template<typename StateType, typename VectorStateType>
  inline
  void DiffusionEvaluator<CoeffType, VectorCoeffType,MatrixCoeffType>::diffusion(const VectorStateType &molar_concentrations,
                                                                 const StateType &z, VectorStateType &omega_a, VectorStateType &omega_b) const
  {

     antioch_assert_equal_to(molar_concentrations.size(),_mixture.neutral_composition().n_species());
     antioch_assert_equal_to(omega_a.size(),_mixture.neutral_composition().n_species());
     antioch_assert_equal_to(omega_b.size(),_mixture.neutral_composition().n_species());

// temperature
     StateType T = _temperature.neutral_temperature(z);
     StateType dT_dz_T = _temperature.dneutral_temperature_dz(z) / T;

// Dtilde
     VectorStateType molecular(molar_concentrations.size());
     _molecular_diffusion.Dtilde(molar_concentrations,T,molecular);// Dtilde

// nTot
     StateType nTot(0);
     for(unsigned int s = 0; s < molar_concentrations.size(); s++)
     {
        nTot += molar_concentrations[s];
     }
     StateType one_over_nTot = Antioch::constant_clone(T,1) / nTot;

// scale heights
     VectorStateType Hs(molar_concentrations.size());
     _mixture.scale_heights(z,Hs);

// eddy diff (K / Ha)
     StateType eddy_K = _eddy_diffusion.K(nTot);
     StateType eddy_K_Ha = eddy_K / _mixture.atmospheric_scale_height(molar_concentrations,z);
     
     StateType eddy_K_times_dT_dz_T = eddy_K * dT_dz_T;

// in cm-3.km.s-1
     for(unsigned int s = 0; s < _mixture.neutral_composition().n_species(); s++)
     {
            // omega_A = -  Dtilde - K
            omega_a[s] = - Antioch::constant_clone(T,1e-10) * (molecular[s] + eddy_K);
            // omega_B = - ( Dtilde / Hs + Dtilde / T * dT_dz * (1 + (nt - ns) / nt) + K/Ha + K / T * dT_dz )
            omega_b[s] = - Antioch::constant_clone(T,1e-10) *
                        (
                           molecular[s] / Hs[s] 
                         + molecular[s] * dT_dz_T * ( Antioch::constant_clone(z,1) + (nTot - molar_concentrations[s]) * one_over_nTot * _mixture.thermal_coefficient()[s] )
                         + eddy_K_Ha
                         + eddy_K_times_dT_dz_T
                        );
     }
     return;
  }

  template <typename CoeffType, typename VectorCoeffType, typename MatrixCoeffType>
  template <typename StateType, typename VectorStateType, typename MatrixStateType>
  inline
  void DiffusionEvaluator<CoeffType, VectorCoeffType,MatrixCoeffType>::diffusion_and_derivs(const VectorStateType &molar_concentrations,
                                                                                            const StateType &z, 
                                                                                            VectorStateType &omegas_A_TERM,
                                                                                            VectorStateType &omegas_B_TERM,
                                                                                            MatrixStateType &domegas_dn_i_A_TERM,
                                                                                            MatrixStateType &domegas_dn_i_B_TERM) const
  {

     antioch_assert_equal_to(_mixture.neutral_composition().n_species(),molar_concentrations.size());
     antioch_assert_equal_to(_mixture.neutral_composition().n_species(),omegas_A_TERM.size());
     antioch_assert_equal_to(_mixture.neutral_composition().n_species(),omegas_B_TERM.size());
     antioch_assert_equal_to(_mixture.neutral_composition().n_species(),domegas_dn_i_A_TERM.size());
     antioch_assert_equal_to(_mixture.neutral_composition().n_species(),domegas_dn_i_B_TERM.size());
//params
     StateType nTot;
     Antioch::set_zero(nTot);
     for(unsigned int s = 0; s < molar_concentrations.size();s++)
     {
        nTot += molar_concentrations[s];
        antioch_assert_equal_to(_mixture.neutral_composition().n_species(),domegas_dn_i_A_TERM[s].size());
        antioch_assert_equal_to(_mixture.neutral_composition().n_species(),domegas_dn_i_B_TERM[s].size());
     }
     StateType T       = _temperature.neutral_temperature(z);
     StateType dT_dz_T = _temperature.dneutral_temperature_dz(z) / T;

//eddy
     StateType dK_dn = _eddy_diffusion.K_deriv_ns(nTot);
     StateType K     = _eddy_diffusion.K(nTot);

//molecular
     VectorStateType Dtilde;
     Dtilde.resize(_mixture.neutral_composition().n_species(),0.);
     MatrixCoeffType dDtilde_dn;
     dDtilde_dn.resize(_mixture.neutral_composition().n_species());
     for(unsigned int s = 0; s < _mixture.neutral_composition().n_species(); s++)
     {
        dDtilde_dn[s].resize(_mixture.neutral_composition().n_species());
     }

     _molecular_diffusion.Dtilde_and_derivs_dn(molar_concentrations,T,nTot,Dtilde,dDtilde_dn);

//scale heights
     CoeffType Ha;
     VectorStateType dHa_dn_i;
     VectorStateType Hs;
     Hs.resize(_mixture.neutral_composition().n_species(),0);
     dHa_dn_i.resize(_mixture.neutral_composition().n_species(),0);

     _mixture.datmospheric_scale_height_dn_i(molar_concentrations,z,Ha,dHa_dn_i);
     _mixture.scale_heights(z,Hs);


// temporaries to limit computations
        // K / Ha
     StateType K_Ha(K / Ha);
          // K / (Ha * Ha)
     StateType K_Ha_2(K_Ha / Ha );
        // K * 1/T * dT_dz
     StateType K_times_dT_dz_T(K * dT_dz_T);
        // dK_dn * (1/Ha + dT_dz * 1/T)
     StateType dK_dn_times_stuff(dK_dn * (1 / Ha + dT_dz_T));

     for(unsigned int s = 0; s < _mixture.neutral_composition().n_species(); s++)
     {

// temporaries to limit computations
          // 1/T * dT_dz * (1 + (1 -xs) * alphas)
        StateType dT_dz_T_times_stuff(dT_dz_T * (1 + ( (nTot - molar_concentrations[s])/nTot) * _mixture.thermal_coefficient()[s]));
          // 1/Hs
        StateType one_Hs(1 / Hs[s]);
          // Dtilde * 1/T * dT_dz * alphas / nTot
        StateType Dtilde_times_stuff(Dtilde[s] * dT_dz_T * _mixture.thermal_coefficient()[s] / nTot);
          // Dtilde * 1/T * dT_dz * alphas * xs / nTot
        StateType Dtilde_times_more_stuff(Dtilde_times_stuff * molar_concentrations[s] / nTot);

        omegas_A_TERM[s] = - Dtilde[s] - K ;
        omegas_B_TERM[s] = - Dtilde[s] * one_Hs               // - D / Hs
                           - Dtilde[s] * dT_dz_T_times_stuff  // - D * 1/T * dT_dz * (1 + (1 -xs) * alphas)
                           - K_Ha                             // - K / Ha
                           - K_times_dT_dz_T;                 // - K * 1 / T * dT_dz 

/*
    omega = omega_A * dn_dz + omega_B * n
    [cm-3.cm2.s-1.km-1] = [cm2.s-1] * [cm-3.km-1] + [cm2.s-1.km-1] * [cm-3]
    we want at the end
    [cm-3.km.s-1], thus 
        omega_A in [km2.s-1] and
        omega_B in [km2.s-1.km-1]
 */

       for(unsigned int i = 0; i < _mixture.neutral_composition().n_species(); i++)
       {
          domegas_dn_i_A_TERM[s][i] = - (dDtilde_dn[s][i] + dK_dn) * Antioch::constant_clone(T,1e-10); //cm2.s-1.cm3 to km2.s-1.cm3
          domegas_dn_i_B_TERM[s][i] = -  dDtilde_dn[s][i] * ( one_Hs + dT_dz_T_times_stuff )
                                      + Dtilde_times_more_stuff // - Dtilde * 1/T * dT_dz * alphas * (- xs / nTot )
                                      - dK_dn_times_stuff
                                      + K_Ha_2 * dHa_dn_i[i];
        if(i == s)domegas_dn_i_B_TERM[s][i] -= Dtilde_times_stuff;
// cm2.s-1.cm3 to km2.s-1.cm3
         domegas_dn_i_B_TERM[s][i] *= Antioch::constant_clone(T,1e-10);
       }
// cm2.s-1 to km2.s-1
       omegas_A_TERM[s] *= Antioch::constant_clone(T,1e-10);
// cm2.km-1.s-1 to km2.km-1.s-1
       omegas_B_TERM[s] *= Antioch::constant_clone(T,1e-10);
     }

     return;

  }


}

#endif

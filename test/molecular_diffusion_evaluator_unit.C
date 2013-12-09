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

//Antioch
#include "antioch/vector_utils_decl.h"
#include "antioch/physical_constants.h"
#include "antioch/sigma_bin_converter.h"
#include "antioch/vector_utils.h"

//Planet
#include "planet/diffusion_evaluator.h"
#include "planet/planet_constants.h"

//C++
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <limits>


template<typename Scalar>
int check_test(Scalar theory, Scalar cal, const std::string &words)
{
  const Scalar tol = std::numeric_limits<Scalar>::epsilon() * 100.L;
  if(std::abs((theory-cal)/theory) < tol)return 0;
  std::cout << std::scientific << std::setprecision(20)
            << "failed test: " << words << "\n"
            << "theory: " << theory
            << "\ncalculated: " << cal
            << "\ndifference: " << std::abs((theory-cal)/cal)
            << "\ntolerance: " << tol << std::endl;
  return 1;
}

template<typename VectorScalar>
void linear_interpolation(const VectorScalar &temp0, const VectorScalar &alt0,
                          const VectorScalar &alt1, VectorScalar &temp1)
{
  unsigned int j(0);
  typename Antioch::value_type<VectorScalar>::type a;
  typename Antioch::value_type<VectorScalar>::type b;
  temp1.resize(alt1.size());
  for(unsigned int iz = 0; iz < alt1.size(); iz++)
  {
     while(alt0[j] < alt1[iz])
     {
        j++;
        if(!(j < alt0.size()))break;
     }
     if(j == 0)
     {
        Antioch::set_zero(a);
        b = temp0[j];
     }else if(j < alt0.size() - 1)
     {
        a = (temp0[j] - temp0[j-1])/(alt0[j] - alt0[j-1]);
        b = temp0[j] - a * alt0[j];
     }else
     {
        Antioch::set_zero(a);
        b = temp0.back();
     }
     temp1[iz] = a * alt1[iz] + b;
  }
}


template<typename Scalar, typename VectorScalar = std::vector<Scalar> >
void read_temperature(VectorScalar &T0, VectorScalar &Tz, const std::string &file)
{
  T0.clear();
  Tz.clear();
  std::string line;
  std::ifstream temp(file);
  getline(temp,line);
  while(!temp.eof())
  {
     Scalar t,tz,dt,dtz;
     temp >> t >> tz >> dt >> dtz;
     T0.push_back(t);
     Tz.push_back(tz);
  }
  temp.close();
  return;
}

template<typename Scalar>
Scalar barometry(const Scalar &zmin, const Scalar &z, const Scalar &T, const Scalar &Mm, const Scalar &botdens)
{
   return botdens * Antioch::ant_exp(-(z - zmin)/((Planet::Constants::Titan::radius<Scalar>() + z) * (Planet::Constants::Titan::radius<Scalar>() + zmin) * 1e3 *
                                             Antioch::Constants::Avogadro<Scalar>() * Planet::Constants::Universal::kb<Scalar>() * T / 
                                                        (Planet::Constants::Universal::G<Scalar>() * Planet::Constants::Titan::mass<Scalar>() * Mm))
                              );
}

template<typename Scalar, typename VectorScalar, typename MatrixScalar>
void calculate_densities(MatrixScalar &densities, const Scalar &tot_dens, const VectorScalar &molar_frac, 
                        const Scalar &zmin,const Scalar &zmax,const Scalar &zstep, 
                        const VectorScalar &T, const VectorScalar &mm)
{
   unsigned int iz(0);
   Scalar Mm;
   Antioch::set_zero(Mm);
   for(unsigned int s = 0; s < molar_frac.size(); s++)
   {
      Mm += molar_frac[s] * mm[s];
   }
   Mm *= 1e-3;//to kg
   densities.clear();
   densities.resize(molar_frac.size());
   for(Scalar z = zmin; z <= zmax; z += zstep)
   {
      for(unsigned int s = 0; s < molar_frac.size(); s++)
      {
        densities[s].push_back(molar_frac[s] * barometry(zmin,z,T[iz],Mm,tot_dens));
      }
      iz++;
   }

   return;
}

template<typename Scalar>
Scalar binary_coefficient(const Scalar &T, const Scalar &P, const Scalar &D01, const Scalar &beta)
{
   return D01 * Planet::Constants::Convention::P_normal<Scalar>() / P * Antioch::ant_pow(T/Planet::Constants::Convention::T_standard<Scalar>(),beta);
}

template<typename Scalar>
Scalar binary_coefficient(const Scalar &Dii, const Scalar &Mi, const Scalar &Mj)
{
   return (Mj < Mi)?Dii * Antioch::ant_sqrt((Mj/Mi + Scalar(1.L))/Scalar(2.L))
                   :
                    Dii * Antioch::ant_sqrt((Mj/Mi));
}

template<typename Scalar>
Scalar pressure(const Scalar &n, const Scalar &T)
{
   return n * 1e6L * Planet::Constants::Universal::kb<Scalar>() * T; //cm-3 -> m-3
}

template <typename Scalar>
int tester(const std::string &input_T)
{
//description
  std::vector<std::string> neutrals;
  std::vector<std::string> ions;
  neutrals.push_back("N2");
  neutrals.push_back("CH4");
  neutrals.push_back("C2H");
//ionic system contains neutral system
  ions = neutrals;
  ions.push_back("N2+");
  Scalar MN(14.008L), MC(12.011), MH(1.008L);
  Scalar MN2 = 2.L*MN , MCH4 = MC + 4.L*MH, MC2H = 2.L * MC + MH;
  std::vector<Scalar> Mm;
  Mm.push_back(MN2);
  Mm.push_back(MCH4);
  Mm.push_back(MC2H);

//densities
  std::vector<Scalar> molar_frac;
  molar_frac.push_back(0.95999L);
  molar_frac.push_back(0.04000L);
  molar_frac.push_back(0.00001L);
  molar_frac.push_back(0.L);
  Scalar dens_tot(1e12L);

//hard sphere radius
  std::vector<Scalar> hard_sphere_radius;
  hard_sphere_radius.push_back(2.0675e-8L * 1e-2L); //N2  in cm -> m
  hard_sphere_radius.push_back(2.3482e-8L * 1e-2L); //CH4 in cm -> m
  hard_sphere_radius.push_back(0.L); //C2H

//zenith angle
//not necessary

//photon flux
//not necessary

////cross-section
//not necessary

//altitudes
  Scalar zmin(600.),zmax(1400.),zstep(10.);

//binary diffusion
  Scalar bCN1(1.04e-5 * 1e-4),bCN2(1.76); //cm2 -> m2
  Planet::DiffusionType CN_model(Planet::DiffusionType::Wakeham);
  Scalar bCC1(5.73e16 * 1e-4),bCC2(0.5); //cm2 -> m2
  Planet::DiffusionType CC_model(Planet::DiffusionType::Wilson);
  Scalar bNN1(0.1783 * 1e-4),bNN2(1.81); //cm2 -> m2
  Planet::DiffusionType NN_model(Planet::DiffusionType::Massman);

/************************
 * first level
 ************************/

//altitude
  Planet::Altitude<Scalar,std::vector<Scalar> > altitude(zmin,zmax,zstep);

//neutrals
  Antioch::ChemicalMixture<Scalar> neutral_species(neutrals); 

//ions
  Antioch::ChemicalMixture<Scalar> ionic_species(ions); 

//chapman
//not needed

//binary diffusion
  Planet::BinaryDiffusion<Scalar> N2N2(   Antioch::Species::N2,  Antioch::Species::N2 , bNN1, bNN2, NN_model);
  Planet::BinaryDiffusion<Scalar> N2CH4(  Antioch::Species::N2,  Antioch::Species::CH4, bCN1, bCN2, CN_model);
  Planet::BinaryDiffusion<Scalar> CH4CH4( Antioch::Species::CH4, Antioch::Species::CH4, bCC1, bCC2, CC_model);
  Planet::BinaryDiffusion<Scalar> N2C2H( Antioch::Species::N2, Antioch::Species::C2H);
  Planet::BinaryDiffusion<Scalar> CH4C2H( Antioch::Species::CH4, Antioch::Species::C2H);
  std::vector<std::vector<Planet::BinaryDiffusion<Scalar> > > bin_diff_coeff;
  bin_diff_coeff.resize(2);
  bin_diff_coeff[0].push_back(N2N2);
  bin_diff_coeff[0].push_back(N2CH4);
  bin_diff_coeff[0].push_back(N2C2H);
  bin_diff_coeff[1].push_back(N2CH4);
  bin_diff_coeff[1].push_back(CH4CH4);
  bin_diff_coeff[1].push_back(CH4C2H);


/************************
 * second level
 ************************/

//temperature
  std::vector<Scalar> T0,Tz;
  read_temperature<Scalar>(T0,Tz,input_T);
  std::vector<Scalar> neutral_temperature;
  linear_interpolation(T0,Tz,altitude.altitudes(),neutral_temperature);
  Planet::AtmosphericTemperature<Scalar, std::vector<Scalar> > temperature(neutral_temperature, neutral_temperature, altitude);

//photon opacity
//not needed

//reaction sets
//not needed

/************************
 * third level
 ************************/

//atmospheric mixture
  Planet::AtmosphericMixture<Scalar,std::vector<Scalar>, std::vector<std::vector<Scalar> > > composition(neutral_species, ionic_species, altitude, temperature);
  composition.init_composition(molar_frac,dens_tot);
  composition.set_hard_sphere_radius(hard_sphere_radius);
  composition.initialize();

//kinetics evaluators
//not needed

/************************
 * fourth level
 ************************/

//photon evaluator
//not needed

//molecular diffusion
  Planet::MolecularDiffusionEvaluator<Scalar,std::vector<Scalar>, std::vector<std::vector<Scalar> > > molecular_diffusion(bin_diff_coeff,
                                                                                                                          composition,
                                                                                                                          altitude,
                                                                                                                          temperature);
  molecular_diffusion.make_molecular_diffusion();

//eddy diffusion
//not needed

/************************
 * checks
 ************************/

  molar_frac.pop_back();//get the ion outta here
  Scalar Matm(0.L);
  for(unsigned int s = 0; s < molar_frac.size(); s++)
  {
     Matm += molar_frac[s] * composition.neutral_composition().M(s);
  }
  Matm *= 1e-3L; //to kg

  std::vector<std::vector<Scalar> > densities;
  calculate_densities(densities, dens_tot, molar_frac, zmin,zmax,zstep, temperature.neutral_temperature(), Mm);
//N2, CH4, C2H
  std::vector<std::vector<Scalar> > Dij;
  Dij.resize(2);
  Dij[0].resize(3,0.L);
  Dij[1].resize(3,0.L);

  int return_flag(0);
  for(unsigned int iz = 0; iz < altitude.altitudes().size(); iz++)
  {
      Scalar P = pressure(composition.total_density()[iz],temperature.neutral_temperature()[iz]);
      Scalar T = temperature.neutral_temperature()[iz];
      Dij[0][0] = binary_coefficient(T,P,bNN1,bNN2); //N2 N2
      Dij[0][1] = binary_coefficient(T,P,bCN1 * Antioch::ant_pow(Planet::Constants::Convention::T_standard<Scalar>(),bCN2),bCN2); //N2 CH4
      Dij[0][2] = binary_coefficient(Dij[0][0],Mm[0],Mm[2]); //N2 C2H
      Dij[1][0] = Dij[0][1]; //CH4 N2
      Dij[1][1] = binary_coefficient(T,P,bCC1 * Antioch::ant_pow(Planet::Constants::Convention::T_standard<Scalar>(),bCC2 + Scalar(1.L)) 
                                              * Planet::Constants::Universal::kb<Scalar>()
                                              / Planet::Constants::Convention::P_normal<Scalar>(),bCC2 + Scalar(1.L)); //CH4 CH4
      Dij[1][2] = binary_coefficient(Dij[1][1],Mm[1],Mm[2]); //CH4 C2H

      return_flag = return_flag ||
                    check_test(Dij[0][0],molecular_diffusion.binary_coefficient(0,0,T,P),"binary molecular coefficient N2 N2 at altitude") || 
                    check_test(Dij[0][1],molecular_diffusion.binary_coefficient(0,1,T,P),"binary molecular coefficient N2 CH4 at altitude") || 
                    check_test(Dij[0][2],molecular_diffusion.binary_coefficient(0,2,T,P),"binary molecular coefficient N2 C2H at altitude") || 
                    check_test(Dij[1][1],molecular_diffusion.binary_coefficient(1,1,T,P),"binary molecular coefficient CH4 CH4 at altitude") || 
                    check_test(Dij[1][2],molecular_diffusion.binary_coefficient(1,2,T,P),"binary molecular coefficient CH4 C2H at altitude");

      for(unsigned int s = 0; s < molar_frac.size(); s++)
      {
        Scalar tmp(0.L);
        for(unsigned int medium = 0; medium < 2; medium++)
        {
           if(s == medium)continue;
           tmp += densities[medium][iz]/Dij[medium][s];
        }
        Scalar Ds = (barometry(zmin,altitude.altitudes()[iz],neutral_temperature[iz],Matm,dens_tot) - densities[s][iz]) / tmp;

        Scalar M_diff(0.L);
        Scalar totdens_diff(0.L);
        for(unsigned int j = 0; j < molar_frac.size(); j++)
        {
           if(s == j)continue;
           M_diff += densities[j][iz] * Mm[j];
           totdens_diff += densities[j][iz];
        }
        M_diff /= totdens_diff;
        Scalar Dtilde = Ds / (Scalar(1.L) - composition.neutral_molar_fraction()[s][iz] * (Scalar(1.L) - composition.neutral_composition().M(s)/M_diff));
        return_flag = return_flag ||
                      check_test(Dtilde,molecular_diffusion.Dtilde()[s][iz],"Dtilde of species at altitude");

      }
  }

  return return_flag;
}

int main(int argc, char** argv)
{
  // Check command line count.
  if( argc < 2 )
    {
      // TODO: Need more consistent error handling.
      std::cerr << "Error: Must specify input file." << std::endl;
      antioch_error();
    }

  return (tester<float>(std::string(argv[1])) ||
          tester<double>(std::string(argv[1])));//||
          //tester<long double>(std::string(argv[1])));
}
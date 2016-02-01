/*
 *@BEGIN LICENSE
 *
 * PSI4: an ab initio quantum chemistry software package
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *@END LICENSE
 */

#include "sapt2p.h"
#include <physconst.h>

namespace psi { namespace sapt {

SAPT2p::SAPT2p(SharedWavefunction Dimer, SharedWavefunction MonomerA,
               SharedWavefunction MonomerB, Options& options,
               boost::shared_ptr<PSIO>psio)
                : SAPT2(Dimer, MonomerA, MonomerB, options, psio),
  e_disp21_(0.0),
  e_disp22sdq_(0.0),
  e_disp22t_(0.0),
  e_est_disp22t_(0.0),
  e_sapt2p_(0.0),
  e_disp2d_ccd_(0.0),
  e_disp22s_ccd_(0.0),
  e_disp22t_ccd_(0.0),
  e_est_disp22t_ccd_(0.0),
  e_sapt2p_ccd_(0.0)
{
  ccd_disp_ = options_.get_bool("DO_CCD_DISP");
  mbpt_disp_  = (!ccd_disp_ ? true : options_.get_bool("DO_MBPT_DISP"));
  ccd_maxiter_ = options_.get_int("CCD_MAXITER");
  max_ccd_vecs_ = options_.get_int("MAX_CCD_DIISVECS");
  min_ccd_vecs_ = options_.get_int("MIN_CCD_DIISVECS");
  ccd_e_conv_ = options_.get_double("CCD_E_CONVERGENCE");
  ccd_t_conv_ = options_.get_double("CCD_T_CONVERGENCE");
  if (ccd_disp_) {
    psio_->open(PSIF_SAPT_CCD,PSIO_OPEN_NEW);
  }
}

SAPT2p::~SAPT2p()
{
  if (ccd_disp_) {
    psio_->close(PSIF_SAPT_CCD,0);
  }
}

double SAPT2p::compute_energy()
{
  print_header();

  timer_on("DF Integrals       ");
    df_integrals();
  timer_off("DF Integrals       ");
  timer_on("Omega Integrals    ");
    w_integrals();
  timer_off("Omega Integrals    ");
  timer_on("Amplitudes         "); 
    amplitudes();
  timer_off("Amplitudes         "); 
  timer_on("Elst10             ");
    elst10();
  timer_off("Elst10             ");
  timer_on("Exch10 S^2         ");
    exch10_s2();
  timer_off("Exch10 S^2         ");
  timer_on("Exch10             ");
    exch10();
  timer_off("Exch10             ");
  timer_on("Ind20,r            ");
    ind20r();
  timer_off("Ind20,r            ");
  timer_on("Exch-Ind20,r       ");
    exch_ind20r();
  timer_off("Exch-Ind20,r       ");
  timer_on("Disp20             ");
    disp20();
  timer_off("Disp20             ");
  timer_on("Exch-Disp20        ");
    exch_disp20();
  timer_off("Exch-Disp20        ");
  timer_on("Elst12             ");
    elst12();
  timer_off("Elst12             ");
  timer_on("Exch11             ");
    exch11();
  timer_off("Exch11             ");
  timer_on("Exch12             ");
    exch12();
  timer_off("Exch12             ");
  timer_on("Ind22              ");
    ind22();
  timer_off("Ind22              ");
  timer_on("Disp21             ");
    disp21();
  timer_off("Disp21             ");

  if (mbpt_disp_) {

  timer_on("Disp22 (SDQ)       ");
    disp22sdq();
  timer_off("Disp22 (SDQ)       ");
  timer_on("Disp22 (T)         ");
    disp22t();
  timer_off("Disp22 (T)         ");

  }

  if (ccd_disp_) {
 
  timer_on("Disp2(CCD)         ");
    disp2ccd();
  timer_off("Disp2(CCD)         ");
  timer_on("Disp22 (T) (CCD)   ");
    disp22tccd();
  timer_off("Disp22 (T) (CCD)   ");

  }

  print_results();

  return (e_sapt0_);
}

void SAPT2p::print_header()
{
  outfile->Printf("        SAPT2+  \n");
  if (ccd_disp_) 
    outfile->Printf("    CCD+(ST) Disp   \n");
  outfile->Printf("    Ed Hohenstein\n") ;
  outfile->Printf("     6 June 2009\n") ;
  outfile->Printf("\n");
  outfile->Printf("      Orbital Information\n");
  outfile->Printf("  --------------------------\n");
  if (nsoA_ != nso_ || nsoB_ != nso_) {
    outfile->Printf("    NSO        = %9d\n",nso_);
    outfile->Printf("    NSO A      = %9d\n",nsoA_);
    outfile->Printf("    NSO B      = %9d\n",nsoB_);
    outfile->Printf("    NMO        = %9d\n",nmo_);
    outfile->Printf("    NMO A      = %9d\n",nmoA_);
    outfile->Printf("    NMO B      = %9d\n",nmoB_);
  } else {
    outfile->Printf("    NSO        = %9d\n",nso_);
    outfile->Printf("    NMO        = %9d\n",nmo_);
  }
  outfile->Printf("    NRI        = %9d\n",ndf_);
  outfile->Printf("    NOCC A     = %9d\n",noccA_);
  outfile->Printf("    NOCC B     = %9d\n",noccB_);
  outfile->Printf("    FOCC A     = %9d\n",foccA_);
  outfile->Printf("    FOCC B     = %9d\n",foccB_);
  outfile->Printf("    NVIR A     = %9d\n",nvirA_);
  outfile->Printf("    NVIR B     = %9d\n",nvirB_);
  outfile->Printf("\n");

  long int mem = (long int) memory_;
  mem /= 8L;
  long int occ = noccA_;
  if (noccB_ > noccA_)
    occ = noccB_;
  long int vir = nvirA_;
  if (nvirB_ > nvirA_)
    vir = nvirB_;
  long int ovov = occ*occ*vir*vir;
  long int vvnri = vir*vir*ndf_;
  double memory = 8.0*(vvnri + ovov*3L)/1000000.0;
  if (ccd_disp_) {
    double ccd_memory = 8.0*(ovov*5L)/1000000.0;
    memory = (memory > ccd_memory ? memory : ccd_memory);
  }

  if (print_) {
    outfile->Printf("    Estimated memory usage: %.1lf MB\n\n",memory);
    
  }
  if (options_.get_bool("SAPT_MEM_CHECK"))
    if (mem < vvnri + ovov*3L) 
      throw PsiException("Not enough memory", __FILE__,__LINE__);

  outfile->Printf("    Natural Orbital Cutoff: %11.3E\n", occ_cutoff_);
  outfile->Printf("    Disp(T3) Truncation:    %11s\n", (nat_orbs_t3_ ? "Yes" : "No"));
  outfile->Printf("    CCD (vv|vv) Truncation: %11s\n", (nat_orbs_v4_ ? "Yes" : "No"));
  outfile->Printf("    MBPT T2 Truncation:     %11s\n", (nat_orbs_t2_ ? "Yes" : "No"));
  outfile->Printf("\n");

  
}

void SAPT2p::print_results()
{
  // The tolerance to scale exchange energies, i.e. if E_exch10 is
  // less than the scaling tolerance, we do not scale.
  double scaling_tol = 1.0e-5;

  // The power applied to the scaling factor for sSAPT0
  double alpha = 3.0;

  double sapt_Xscal = ( e_exch10_ < scaling_tol ? 1.0 : e_exch10_ / e_exch10_s2_ );
  double sSAPT_Xscal = pow(sapt_Xscal,alpha);

  // Now we compute everything once without scaling, and then with scaling.
  std::vector<double> Xscal;
  Xscal.push_back(1.0);
  Xscal.push_back(sapt_Xscal);

  // Grab the supermolecular MP2 correlation energy if it is here
  double e_MP2 = Process::environment.globals["SA MP2 CORRELATION ENERGY"];

  // The main loop, computes everything with all scaling factors in
  // the Xscal vector. Only exports variables once, for the scaling factor
  // of 1.0.
  std::vector<double>::iterator scal_it;
  for(scal_it = Xscal.begin(); scal_it != Xscal.end(); ++scal_it) {

    double dHF2 = eHF_ - (e_elst10_ + e_exch10_ + e_ind20_ + *scal_it * e_exch_ind20_);

    double dMP2_2 = 0.0;
    if(e_MP2 != 0.0) {
        dMP2_2 = e_MP2 - ( e_elst12_ + e_ind22_ + e_disp20_ +
                *scal_it * ( e_exch11_ + e_exch12_ + e_exch_ind22_ + e_exch_disp20_));
    }

    double e_disp_mp4 = *scal_it * e_exch_disp20_ + e_disp20_ + e_disp21_ + e_disp22sdq_;
    if(nat_orbs_t3_) {
        e_disp_mp4 += e_est_disp22t_;
    } else {
        e_disp_mp4 += e_disp22t_;
    }

    double e_disp_ccd = 0.0;
    if (ccd_disp_) {
      e_disp_ccd = *scal_it * e_exch_disp20_;
      e_disp_ccd += e_disp2d_ccd_ + e_disp22s_ccd_;
      if (nat_orbs_t3_) {
        e_disp_ccd += e_est_disp22t_ccd_;
      } else {
        e_disp_ccd += e_disp22t_ccd_;
      }
    }
    
    e_sapt0_ = e_elst10_ + e_exch10_ + dHF2 + e_ind20_ + e_disp20_ + 
               *scal_it * (e_exch_ind20_ + e_exch_disp20_);
    double e_sSAPT0 = 0.0;
    if( *scal_it == sapt_Xscal) {
      e_sSAPT0 = e_elst10_ + e_exch10_ + e_ind20_ + dHF2 + sapt_Xscal * e_exch_ind20_ +
                 e_disp20_ + sSAPT_Xscal * e_exch_disp20_ + (sSAPT_Xscal - 1.0) * e_exch_ind20_;
    }
    e_sapt2_ = e_elst10_ + e_exch10_ + dHF2 + e_ind20_ + e_disp20_ + 
               *scal_it * (e_exch_ind20_ + e_exch_disp20_) + 
               e_elst12_ + *scal_it * (e_exch11_ + e_exch12_ + e_exch_ind22_) + e_ind22_; 

    e_sapt2p_ = e_elst10_ + e_exch10_ + dHF2 + e_ind20_ + *scal_it * (e_exch_ind20_ ) + 
                e_elst12_ + *scal_it * (e_exch11_ + e_exch12_ + e_exch_ind22_) + 
                e_ind22_ + e_disp_mp4;

    double e_sapt2p_ccd_dmp2 = 0.0;
    if(ccd_disp_) {
      e_sapt2p_ccd_ = e_elst10_ + e_exch10_ + dHF2 + e_ind20_ + *scal_it * (e_exch_ind20_ ) +
                  e_elst12_ + *scal_it * (e_exch11_ + e_exch12_ + e_exch_ind22_) + 
                  e_ind22_ + e_disp_ccd;
      if(e_MP2 != 0.0) {
        e_sapt2p_ccd_dmp2 = e_sapt2p_ccd_ + dMP2_2;
      }
    }

    double e_sapt2p_dmp2 = 0.0;
    if(e_MP2 != 0.0) {
      e_sapt2p_dmp2 = e_sapt2p_ + dMP2_2;
    }

    double tot_elst = e_elst10_ + e_elst12_;
    double tot_exch = e_exch10_ + *scal_it * (e_exch11_ + e_exch12_);
    double tot_ind = e_ind20_ + dHF2 + e_ind22_ 
            + *scal_it * (e_exch_ind20_ + e_exch_ind22_);
    double tot_ct = e_ind20_ + e_ind22_ +
            *scal_it * (e_exch_ind20_ + e_exch_ind22_);
    double tot_disp = 0.0;
    if (nat_orbs_t3_)
      tot_disp = e_disp20_ + e_disp21_ + e_disp22sdq_ + e_est_disp22t_ +
              *scal_it * e_exch_disp20_;
    else
      tot_disp = e_disp20_ + e_disp21_ + e_disp22sdq_ + e_disp22t_ +
              *scal_it * e_exch_disp20_;
  
    if (ccd_disp_) {
      tot_disp = 0.0;
      if (nat_orbs_t3_)
        tot_disp = e_disp2d_ccd_ + e_disp22s_ccd_ + e_est_disp22t_ccd_ + *scal_it * e_exch_disp20_;
      else
        tot_disp = e_disp2d_ccd_ + e_disp22s_ccd_ + e_disp22t_ccd_ + *scal_it * e_exch_disp20_;
    }
  
    if(scal_it == Xscal.begin()) {
        outfile->Printf("\n    SAPT Results ==> NO EXCHANGE SCALING APPLIED <==  \n");
    } else {
        outfile->Printf("\n    SAPT Results ==> ALL S2 TERMS SCALED (see Manual) <== \n");
        outfile->Printf("\n    Scaling factor: %12.6f  \n", *scal_it);
    }
    std::string scaled = (scal_it != Xscal.begin() ? "scal." : "     ");
    outfile->Printf("  --------------------------------------------------------------------------\n");
    outfile->Printf("    Electrostatics          %16.8lf mH %16.8lf kcal mol^-1\n",
      tot_elst*1000.0,tot_elst*pc_hartree2kcalmol);
    outfile->Printf("      Elst10,r              %16.8lf mH %16.8lf kcal mol^-1\n",
      e_elst10_*1000.0,e_elst10_*pc_hartree2kcalmol);
    outfile->Printf("      Elst12,r              %16.8lf mH %16.8lf kcal mol^-1\n",
      e_elst12_*1000.0,e_elst12_*pc_hartree2kcalmol);
    outfile->Printf("\n    Exchange %5s          %16.8lf mH %16.8lf kcal mol^-1\n",
      scaled.c_str(), tot_exch*1000.0,tot_exch*pc_hartree2kcalmol);
    outfile->Printf("      Exch10                %16.8lf mH %16.8lf kcal mol^-1\n",
      e_exch10_*1000.0,e_exch10_*pc_hartree2kcalmol);
    outfile->Printf("      Exch10(S^2)           %16.8lf mH %16.8lf kcal mol^-1\n",
      e_exch10_s2_*1000.0,e_exch10_s2_*pc_hartree2kcalmol);
    outfile->Printf("      Exch11(S^2) %5s     %16.8lf mH %16.8lf kcal mol^-1\n",
      scaled.c_str(), *scal_it * e_exch11_*1000.0,*scal_it * e_exch11_*pc_hartree2kcalmol);
    outfile->Printf("      Exch12(S^2) %5s     %16.8lf mH %16.8lf kcal mol^-1\n",
      scaled.c_str(), *scal_it * e_exch12_*1000.0,*scal_it * e_exch12_*pc_hartree2kcalmol);
    outfile->Printf("\n    Induction %5s         %16.8lf mH %16.8lf kcal mol^-1\n",
      scaled.c_str(), tot_ind*1000.0,tot_ind*pc_hartree2kcalmol);
    outfile->Printf("      Ind20,r               %16.8lf mH %16.8lf kcal mol^-1\n",
      e_ind20_*1000.0,e_ind20_*pc_hartree2kcalmol);
    outfile->Printf("      Ind22                 %16.8lf mH %16.8lf kcal mol^-1\n",
      e_ind22_*1000.0,e_ind22_*pc_hartree2kcalmol);
    outfile->Printf("      Exch-Ind20,r %5s    %16.8lf mH %16.8lf kcal mol^-1\n",
      scaled.c_str(), *scal_it * e_exch_ind20_*1000.0,*scal_it * e_exch_ind20_*pc_hartree2kcalmol);
    outfile->Printf("      Exch-Ind22 %5s      %16.8lf mH %16.8lf kcal mol^-1\n",
      scaled.c_str(), *scal_it * e_exch_ind22_*1000.0,*scal_it * e_exch_ind22_*pc_hartree2kcalmol);
    outfile->Printf("      delta HF,r (2) %5s  %16.8lf mH %16.8lf kcal mol^-1\n",
      scaled.c_str(), dHF2*1000.0,dHF2*pc_hartree2kcalmol);
    if(e_MP2 != 0.0) {
        outfile->Printf("      delta MP2,r (2) %5s %16.8lf mH %16.8lf kcal mol^-1\n",
          scaled.c_str(), dMP2_2*1000.0,dMP2_2*pc_hartree2kcalmol);
    }
    outfile->Printf("\n    Dispersion %5s        %16.8lf mH %16.8lf kcal mol^-1\n",
      scaled.c_str(), tot_disp*1000.0,tot_disp*pc_hartree2kcalmol);
    outfile->Printf("      Disp20                %16.8lf mH %16.8lf kcal mol^-1\n",
      e_disp20_*1000.0,e_disp20_*pc_hartree2kcalmol);
    outfile->Printf("      Disp21                %16.8lf mH %16.8lf kcal mol^-1\n",
      e_disp21_*1000.0,e_disp21_*pc_hartree2kcalmol);
    if (mbpt_disp_) {
      outfile->Printf("      Disp22 (SDQ)          %16.8lf mH %16.8lf kcal mol^-1\n",
        e_disp22sdq_*1000.0,e_disp22sdq_*pc_hartree2kcalmol);
      outfile->Printf("      Disp22 (T)            %16.8lf mH %16.8lf kcal mol^-1\n",
        e_disp22t_*1000.0,e_disp22t_*pc_hartree2kcalmol);
      if (nat_orbs_t3_)
        outfile->Printf("      Est. Disp22 (T)       %16.8lf mH %16.8lf kcal mol^-1\n",
          e_est_disp22t_*1000.0,e_est_disp22t_*pc_hartree2kcalmol);
    }
    if (ccd_disp_) {
      outfile->Printf("      Disp2 (CCD)           %16.8lf mH %16.8lf kcal mol^-1\n",
        e_disp2d_ccd_*1000.0,e_disp2d_ccd_*pc_hartree2kcalmol);
      outfile->Printf("      Disp22 (S) (CCD)      %16.8lf mH %16.8lf kcal mol^-1\n",
        e_disp22s_ccd_*1000.0,e_disp22s_ccd_*pc_hartree2kcalmol);
      outfile->Printf("      Disp22 (T) (CCD)      %16.8lf mH %16.8lf kcal mol^-1\n",
        e_disp22t_ccd_*1000.0,e_disp22t_ccd_*pc_hartree2kcalmol);
      if (nat_orbs_t3_)
        outfile->Printf("      Est. Disp22 (T) (CCD) %16.8lf mH %16.8lf kcal mol^-1\n",
          e_est_disp22t_ccd_*1000.0,e_est_disp22t_ccd_*pc_hartree2kcalmol);
    }
    outfile->Printf("      Exch-Disp20 %5s     %16.8lf mH %16.8lf kcal mol^-1\n",
      scaled.c_str(), *scal_it * e_exch_disp20_*1000.0,*scal_it * e_exch_disp20_*pc_hartree2kcalmol);
  
    outfile->Printf("\n  Total HF                      %16.8lf mH %16.8lf kcal mol^-1\n",
      eHF_*1000.0,eHF_*pc_hartree2kcalmol);
    outfile->Printf("  Total SAPT0 %5s             %16.8lf mH %16.8lf kcal mol^-1\n",
      scaled.c_str(), e_sapt0_*1000.0,e_sapt0_*pc_hartree2kcalmol);
    if(*scal_it == sapt_Xscal && (scal_it != Xscal.begin()) ) {
          outfile->Printf("  Total sSAPT0                  %16.8lf mH %16.8lf kcal mol^-1\n",
          e_sSAPT0*1000.0,e_sSAPT0*pc_hartree2kcalmol);
    }
    outfile->Printf("  Total SAPT2 %5s             %16.8lf mH %16.8lf kcal mol^-1\n",
      scaled.c_str(), e_sapt2_*1000.0,e_sapt2_*pc_hartree2kcalmol);
    if (mbpt_disp_) {
      outfile->Printf("  Total SAPT2+ %5s            %16.8lf mH %16.8lf kcal mol^-1\n",
        scaled.c_str(), e_sapt2p_*1000.0,e_sapt2p_*pc_hartree2kcalmol);
      if(e_MP2 != 0.0) {
          outfile->Printf("  Total SAPT2+(dMP2) %5s      %16.8lf mH %16.8lf kcal mol^-1\n",
            scaled.c_str(), e_sapt2p_dmp2*1000.0,e_sapt2p_dmp2*pc_hartree2kcalmol);
      }
    }
    if (ccd_disp_) {
      outfile->Printf("  Total SAPT2+(CCD) %5s       %16.8lf mH %16.8lf kcal mol^-1\n",
        scaled.c_str(), e_sapt2p_ccd_*1000.0,e_sapt2p_ccd_*pc_hartree2kcalmol);
      if(e_MP2 != 0.0) {
          outfile->Printf("  Total SAPT2+(CCD)(dMP2) %5s %16.8lf mH %16.8lf kcal mol^-1\n",
            scaled.c_str(), e_sapt2p_ccd_dmp2*1000.0,e_sapt2p_ccd_dmp2*pc_hartree2kcalmol);
      }
    }
    outfile->Printf("  --------------------------------------------------------------------------\n");

    // Only export if not scaled.
    if(scal_it == Xscal.begin()) {

        Process::environment.globals["SAPT ELST ENERGY"] = tot_elst;
        Process::environment.globals["SAPT ELST10,R ENERGY"] = e_elst10_;
        Process::environment.globals["SAPT ELST12,R ENERGY"] = e_elst12_;

        Process::environment.globals["SAPT EXCH ENERGY"] = tot_exch;
        Process::environment.globals["SAPT EXCH10 ENERGY"] = e_exch10_;
        Process::environment.globals["SAPT EXCH10(S^2) ENERGY"] = e_exch10_s2_;
        Process::environment.globals["SAPT EXCH11(S^2) ENERGY"] = e_exch11_;
        Process::environment.globals["SAPT EXCH12(S^2) ENERGY"] = e_exch12_;

        Process::environment.globals["SAPT IND ENERGY"] = tot_ind;
        Process::environment.globals["SAPT IND20,R ENERGY"] = e_ind20_;
        Process::environment.globals["SAPT IND22 ENERGY"] = e_ind22_;
        Process::environment.globals["SAPT EXCH-IND20,R ENERGY"] = e_exch_ind20_;
        Process::environment.globals["SAPT EXCH-IND22 ENERGY"] = e_exch_ind22_;
        Process::environment.globals["SAPT HF TOTAL ENERGY"] = eHF_;

        Process::environment.globals["SAPT CT ENERGY"] = tot_ct;

        Process::environment.globals["SAPT DISP ENERGY"] = tot_disp;
        Process::environment.globals["SAPT DISP20 ENERGY"] = e_disp20_;
        Process::environment.globals["SAPT DISP21 ENERGY"] = e_disp21_;
        Process::environment.globals["SAPT EXCH-DISP20 ENERGY"] = e_exch_disp20_;

        Process::environment.globals["SAPT SAPT0 ENERGY"] = e_sapt0_;
        Process::environment.globals["SAPT SAPT2 ENERGY"] = e_sapt2_;

        if (mbpt_disp_) {
            Process::environment.globals["SAPT DISP22(SDQ) ENERGY"] = e_disp22sdq_;
            Process::environment.globals["SAPT DISP22(T) ENERGY"] = e_disp22t_;
            Process::environment.globals["SAPT SAPT2+ ENERGY"] = e_sapt2p_;
            if (nat_orbs_t3_) {
                Process::environment.globals["SAPT EST.DISP22(T) ENERGY"] = e_est_disp22t_;
            } else {
                Process::environment.globals["SAPT EST.DISP22(T) ENERGY"] = e_disp22t_;
            }
            Process::environment.globals["SAPT ENERGY"] = e_sapt2p_;
            Process::environment.globals["CURRENT ENERGY"] = Process::environment.globals["SAPT ENERGY"];
        }

        if (ccd_disp_) {
            Process::environment.globals["SAPT DISP2(CCD) ENERGY"] = e_disp2d_ccd_;
            Process::environment.globals["SAPT DISP22(S)(CCD) ENERGY"] = e_disp22s_ccd_;
            Process::environment.globals["SAPT DISP22(T)(CCD) ENERGY"] = e_disp22t_ccd_;
            Process::environment.globals["SAPT SAPT2+(CCD) ENERGY"] = e_sapt2p_ccd_;
            Process::environment.globals["SAPT ENERGY"] = e_sapt2p_ccd_;
            if (nat_orbs_t3_) {
                Process::environment.globals["SAPT EST.DISP22(T)(CCD) ENERGY"] = e_est_disp22t_ccd_;
            } else {
                Process::environment.globals["SAPT EST.DISP22(T)(CCD) ENERGY"] = e_disp22t_ccd_;
            }
            Process::environment.globals["CURRENT ENERGY"] = Process::environment.globals["SAPT ENERGY"];
        }
    }
  }

}

}}

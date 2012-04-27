/* ****************************************************************************

    Copyright (C) 2004-2011  Christoph Helmberg

    ConicBundle, Version 0.3.10
    File:  CBsources/diagscaling.cxx

    This file is part of ConciBundle, a C/C++ library for convex optimization.

    ConicBundle is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    ConicBundle is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

***************************************************************************** */



#include <math.h>
#include <stdlib.h>
#include "diagscaling.hxx"
#include "mymath.hxx"
#include "sparssym.hxx"

 
using namespace CH_Matrix_Classes;

namespace ConicBundle {


// *****************************************************************************
//                       BundleDiagonalScaling::set_weightu
// *****************************************************************************

void BundleDiagonalScaling::set_weightu(CH_Matrix_Classes::Real in_weightu)
{
  if (fabs(weightu-in_weightu)>1e-10*fabs(weightu)){
    D*=in_weightu/weightu;
    weightu=in_weightu;
  }
}

// *****************************************************************************
//                       BundleDiagonalScaling::norm_sqr
// *****************************************************************************

Real BundleDiagonalScaling::norm_sqr(const Matrix& B) const
{
  Real ipsum=normDsquared(B,D);
  assert(fabs(ipsum-ip(B,sparseDiag(D)*B))<1e-6);
  return ipsum;
}

// *****************************************************************************
//                       BundleDiagonalScaling::dnorm_sqr
// *****************************************************************************

Real BundleDiagonalScaling::dnorm_sqr(const Matrix& B) const
{
  Real ipsum=normDsquared(B,D,0,1);
  assert(fabs(ipsum-ip(B,sparseDiag(inv(D))*B))<1e-6);
  return ipsum;
}

// *****************************************************************************
//                       BundleDiagonalScaling::compute_first_eta
// *****************************************************************************


int BundleDiagonalScaling::compute_first_eta(Matrix& eta,
					     const Matrix& subg,
					     const Matrix& y,
					     const Matrix& lby,
					     const Matrix& uby,
					     const Indexmatrix& bound_index,
					     const Indexmatrix& yfixed) const
{
  bool yf=(yfixed.dim()==y.dim());
  if (bound_index.dim()<y.dim()/2){ 
      
    //bounds on "few" variables
    for(Integer i=0;i<bound_index.dim();i++){
      Integer bind=bound_index(i);
      if ((!yf)||(yfixed(bind)==0)){
	Real s=1./D(bind);
	Real d=y(bind)-subg(bind)*s;
	if (d<lby(bind)){
	  eta(bind)=(lby(bind)-d)/s;
	  continue;
	}
	if (d>uby(bind)){
	  eta(bind)=(uby(bind)-d)/s;
	  continue;
	}
      }
      eta(bind)=0.;
    }
  }
    
  else {
      
    //bounds on "all" variables
    //newy=y-subg%(D^(-1));
    eta.init(subg);
    eta/=D;
    eta+=y;
    for(Integer i=0;i<y.dim();i++){
      if ((!yf)||(yfixed(i)==0)){
	if (lby(i)>eta(i)){
	  eta(i)=(lby(i)-eta(i))*D(i);
	  continue;
	}
	if (uby(i)<eta(i)){
	  eta(i)=(uby(i)-eta(i))*D(i);
	  continue;
	}
      }
      eta(i)=0.;
    }	  
  } //end bounds on "all"
  
  return 0;
}
  

// *****************************************************************************
//                       BundleDiagonalScaling::update_eta_step
// *****************************************************************************


int BundleDiagonalScaling::update_eta_step(Matrix& newy,
				     Matrix& eta,
				     Indexmatrix& update_index,
				     Matrix& update_value,
				     Real& normsubg2,
				     const Matrix& subg,
				     const Matrix& y,
				     const Matrix& lby,
				     const Matrix& uby,
				     const Indexmatrix& bound_index,
				     const Indexmatrix& yfixed) const
{
  newy.init(subg,-1.);
  newy/=D;
  if (yfixed.dim()==newy.dim()){
    for(Integer i=0;i<yfixed.dim();i++){
      if (yfixed(i)) 
	newy(i)=0.;
    }
  }
  newy+=y;
   
  //--- update eta, if necessary 
  //    (so that the step satisfies the box constraints)
  if (bound_index.dim()==0){
    update_value.newsize(0,0);
    update_index.newsize(0,0);
  }
  else if (bound_index.dim()<y.dim()/2){ 
    //bounds on "few" variables
    update_value.newsize(y.dim(),1); chk_set_init(update_value,1);
    update_index.newsize(y.dim(),1); chk_set_init(update_index,1);
      
    Integer nz=0;
    for(Integer i=0;i<bound_index.dim();i++){
      Integer bind=bound_index(i);
      Real old_eta=eta(bind);
      Real d=newy(bind);
      if (d<lby(bind)){
	eta(bind)=(lby(bind)-d)*D(bind);
	newy(bind)=lby(bind);
      }
      else if (d>uby(bind)){
	eta(bind)=(uby(bind)-d)*D(bind);
	newy(bind)=uby(bind);
      }
      else if (old_eta==0.) continue;
      else {
	eta(bind)=0.;
      }
      d=eta(bind)-old_eta;
      if (d!=0.){
	update_value(nz)=d;
	update_index(nz)=bind;
	nz++;
      }
    }
    update_value.reduce_length(nz);
    update_index.reduce_length(nz);
  }
  else {
    //bounds on "all" variables
    update_value.newsize(y.dim(),1); chk_set_init(update_value,1);
    update_index.newsize(y.dim(),1); chk_set_init(update_index,1);

    Integer nz=0;
    for(Integer i=0;i<y.dim();i++){
      Real old_eta=eta(i);
      if (lby(i)>newy(i)){
	eta(i)=(lby(i)-newy(i))*D(i);
	newy(i)=lby(i);
      }
      else if (uby(i)<newy(i)){
	eta(i)=(uby(i)-newy(i))*D(i);
	newy(i)=uby(i);
      }
      else if (old_eta==0.) continue;
      else {
	eta(i)=0.;
      }
      Real d=eta(i)-old_eta;
      if (d!=0.){
	update_value(nz)=d;
	update_index(nz)=i;
	nz++;
      }
    }	  
    update_value.reduce_length(nz);
    update_index.reduce_length(nz);
  } //end bounds on "all"
    
  newy-=y;
  normsubg2=normDsquared(newy,D);
  newy+=y;
  
  return 0;
}    


// *****************************************************************************
//                       BundleDiagonalScaling::compute_QP_costs
// *****************************************************************************


int BundleDiagonalScaling::compute_QP_costs(Symmatrix& Q,
				      Matrix& d,
				      Real& offset,
				      const BundleQPData* datap,
				      const Integer xdim,
				      const Matrix& y,
				      const Matrix& lby,
				      const Matrix& uby,
				      const Matrix& eta,
				      const Indexmatrix& yfixed) const
{
  //--- get linear cost matrix  
  d.init(xdim,1,0.);
  offset=0;
  if (datap->get_row(-1,d,offset,0)){
    if (out)
      (*out)<<"*** ERROR in BundleDiagonalScaling::update_QP_costs(...):  datap->get_row(...) failed"<<std::endl;
    return 1;
  }

  //--- for each y_index 
  Q.init(xdim,0.);
  Matrix tmpvec(xdim,1,0.);
  bool yf= (yfixed.dim()==y.dim());

  for (Integer j=0;j<y.dim();j++){ 

    Real bb=eta(j);
    if (bb<0.) offset+=bb*uby(j);
    else if (bb>0.) offset+=bb*lby(j);
    
    //get cost row corresponding to j
    if (yf && yfixed(j)){
      if (y(j)!=0.){ //(for y==0. there is nothing to do)
	bb=0.;
	if (datap->get_row(j,tmpvec,bb,0)){
	  if (out)
	    (*out)<<"*** ERROR in BundleDiagonalScaling::update_QP_costs(...):  datap->get_row(...) failed"<<std::endl;
	  return 1;
	}
	
	//add offset and linear term
	offset+=bb*y(j);
	d.xpeya(tmpvec,y(j));
      }
    }
    else{
      bb=-bb;
      if (datap->get_row(j,tmpvec,bb,0)){
	if (out)
	  (*out)<<"*** ERROR in BundleDiagonalScaling::update_QP_costs(...):  datap->get_row(...) failed"<<std::endl;
	return 1;
      }
      
      Real by=-bb/D(j);
      offset+=bb*(y(j)+by/2.);
      by+=y(j);
      d.xpeya(tmpvec,by);
      rankadd(tmpvec,Q,1./D(j),1.);
    }
  }

  //cout<<"offset="<<offset<<std::endl;
  //cout<<"d="<<d<<std::endl;
  //cout<<"Q="<<Q<<std::endl;  
 
  return 0;
}


// *****************************************************************************
//                       BundleDiagonalScaling::update_QP_costs
// *****************************************************************************

int BundleDiagonalScaling::update_QP_costs(
					   Matrix& delta_d,  
					   Real& delta_offset,
					   const BundleQPData* datap,
					   const Integer xdim,
					   const Matrix& y,
					   const Matrix& lby,
					   const Matrix& uby,
					   const Matrix& eta,
					   const Indexmatrix& update_index,
					   const Matrix& update_value) const
{
  delta_d.init(xdim,1,0.);
  Matrix tmpvec(xdim,1,0.);
  delta_offset=0;

  //--- for each y_index 
  for (Integer j=0;j<update_index.dim();j++){
    Integer ind=update_index(j);
    
    Real uv=update_value(j);
    Real new_eta=eta(ind);
    Real old_eta=new_eta-uv;
    if (new_eta<0.) {
      if (old_eta>0.){
	delta_offset+=new_eta*uby(ind)-old_eta*lby(ind);
      }
      else if (old_eta==0.){
	delta_offset+=new_eta*uby(ind);
      }
    }
    else if (new_eta>0.) {
      if (old_eta<0.){
	delta_offset+=new_eta*lby(ind)-old_eta*uby(ind);
      }
      else if (old_eta==0.){
	delta_offset+=new_eta*lby(ind);
      }
    }

    //get row corresponding to j
    Real bb=0.;
    if (datap->get_row(ind,tmpvec,bb,0)){
      if (out)
	(*out)<<"*** ERROR in BundleDiagonalScaling::update_QP_costs(...):  datap->get_row(...) failed"<<std::endl;
      return 1;
    }
    delta_offset+=uv*((bb-old_eta-uv/2.)/D(ind)-y(ind));
    
    //add linear term
    delta_d.xpeya(tmpvec,uv/D(ind));
  }

  return 0;
}



}  //namespace ConicBundle

/*
 *  OpenNL<T>: Generic Numerical Library
 *  Copyright (C) 2004 Bruno Levy
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  In addition, as a special exception, the INRIA gives permission to link the 
 *  code of this program with the CGAL library, and distribute linked combinations 
 *  including the two. You must obey the GNU General Public License in all respects 
 *  for all of the code used other than CGAL. 
 *
 *  If you modify this software, you should include a notice giving the
 *  name of the person performing the modification, the date of modification,
 *  and the reason for such modification.
 *
 *  Contact: Bruno Levy
 *
 *     levy@loria.fr
 *
 *     ISA-ALICE Project
 *     LORIA, INRIA Lorraine, 
 *     Campus Scientifique, BP 239
 *     54506 VANDOEUVRE LES NANCY CEDEX 
 *     FRANCE
 *
 *  Note that the GNU General Public License does not permit incorporating
 *  the Software into proprietary programs. 
 *
 *  Laurent Saboret 01/2005: Change for CGAL:
 *		- Added OpenNL namespace
 *		- solve() returns true on success
 *		- test divisions by zero
 */

#ifndef __BICGSTAB__
#define __BICGSTAB__

#include <cmath>
#include <cfloat>
#include <climits>
#include <cassert>

namespace OpenNL {


// Utility macro to display a variable's value
// Usage: x=3.7; cerr << STREAM_TRACE(x) << endl;
//        prints
//        x=3.7
#define STREAM_TRACE(var) #var << "=" << var << " "


/**
 *  The BICGSTAB algorithm without preconditioner:
 *  Ashby, Manteuffel, Saylor
 *     A taxononmy for conjugate gradient methods
 *     SIAM J Numer Anal 27, 1542-1568 (1990)
 *
 * This implementation is inspired by the lsolver library,
 * by Christian Badura, available from:
 * http://www.mathematik.uni-freiburg.de/IAM/Research/projectskr/lin_solver/
 *
  * @param A generic matrix, a function
 *   mult(const MATRIX& M, const double* x, double* y)
 *   needs to be defined.
 * @param b right hand side of the system.
 * @param x initial value.
 * @param eps threshold for the residual.
 * @param max_iter maximum number of iterations.
  */    

template < class MATRIX, class VECTOR> class Solver_BICGSTAB {
public:
    typedef MATRIX Matrix ;
    typedef VECTOR Vector ;
    typedef typename Vector::CoeffType CoeffType ;

    Solver_BICGSTAB() {
		// LS 03/2005: change epsilon from 1e-6 to 1e-4 to parameterize venus-loop.off w/ authalic/square method
        epsilon_ = 1e-4 ;										
        max_iter_ = 0 ;
    }

    void set_epsilon(CoeffType eps) { epsilon_ = eps ; }
    void set_max_iter(unsigned int max_iter) { max_iter_ = max_iter ; }

	// Solve the sparse linear system "A*x = b". Return true on success.
	// 
	// Preconditions:
	// * A.dimension() == b.dimension()
	// * A.dimension() == x.dimension()
	bool solve(const MATRIX &A, const VECTOR& b, VECTOR& x)
	{
        if (A.dimension() != b.dimension())
			return false;
        if (A.dimension() != x.dimension())
			return false;
        if (A.dimension() <= 0)
			return false;

        unsigned int n = A.dimension() ;						// (Square) matrix dimension
        unsigned int max_iter = max_iter_ ;						// Max number of iterations
        if(max_iter == 0) {
			// LS 03/2005: change max_iter from 5*n to 10*n to parameterize venus-loop.off w/ authalic/square method
            max_iter = 10*n ;	
        }
        Vector rT(n) ;											// Initial residue rT=Ax-b
        Vector d(n) ;
        Vector h(n) ;
        Vector u(n) ;
        Vector Ad(n) ;
        Vector t(n) ;
        Vector& s = h ;
        CoeffType rTh, rTAd, rTr, alpha, beta, omega, st, tt;
        unsigned int its=0;										// Loop counter
        CoeffType err=epsilon_*epsilon_*BLAS<Vector>::dot(b,b);	// Error to reach

        Vector r(n) ;											// Current residue r=Ax-b
        mult(A,x,r);
        BLAS<Vector>::axpy(-1,b,r);

	    // Initially, d=h=rT=r=Ax-b
        BLAS<Vector>::copy(r,d);								// d = r
        BLAS<Vector>::copy(d,h);								// h = d
        BLAS<Vector>::copy(h,rT);								// rT = h
        assert( ! IsZero( BLAS<Vector>::dot(rT,rT) ) );

		rTh=BLAS<Vector>::dot(rT,h);							// rTh = (rT|h)
        rTr=BLAS<Vector>::dot(r,r);								// Current error rTr = (r|r)

		while ( rTr>err && its < max_iter) 
		{
			mult(A,d,Ad);										// Ad = A*d
            rTAd=BLAS<Vector>::dot(rT,Ad);						// rTAd = (rT|Ad)
			assert( ! IsZero(rTAd) );
			alpha=rTh/rTAd;										// alpha = rTh/rTAd
            BLAS<Vector>::axpy(-alpha,Ad,r);					// r = r - alpha*Ad
            BLAS<Vector>::copy(h,s);
            BLAS<Vector>::axpy(-alpha,Ad,s);					// h = h - alpha*Ad
            mult(A,s,t);										// t = A*h
            BLAS<Vector>::axpy(1,t,u);
            BLAS<Vector>::scal(alpha,u);
            st=BLAS<Vector>::dot(s,t);							// st = (h|t)
            tt=BLAS<Vector>::dot(t,t);							// tt = (t|t)
			if ( IsZero(st) || IsZero(tt) )
                omega = 0 ;
		    else
	      		omega = st/tt;									// omega = st/tt
            BLAS<Vector>::axpy(-alpha,d,x);						// x = x - alpha*d
            BLAS<Vector>::axpy(-omega,s,x);						// x = x - omega*h
            BLAS<Vector>::copy(s,h);
            BLAS<Vector>::axpy(-omega,t,r);						// r = r - omega*t
	        rTr=BLAS<Vector>::dot(r,r);							// Current error rTr = (r|r)
            BLAS<Vector>::axpy(-omega,t,h);						// h = h - omega*t
			if( IsZero(omega) )									// LS 03/2005: avoid division by zero 
				break;											// stop solver
			if( IsZero(rTh) )									// LS 03/2005: avoid division by zero 
				break;											// stop solver
            beta=(alpha/omega)/rTh; 
			rTh=BLAS<Vector>::dot(rT,h); 						// rTh = (rT|h)
			beta*=rTh;											// beta = (rTh / previous rTh) * (alpha / omega)
            BLAS<Vector>::scal(beta,d);
            BLAS<Vector>::axpy(1,h,d);
            BLAS<Vector>::axpy(-beta*omega,Ad,d);
            its++ ;
        }

		bool success = (rTr <= err);
		std::cerr << STREAM_TRACE(success) << "(" << STREAM_TRACE(its) << STREAM_TRACE(max_iter) << STREAM_TRACE(rTr) << STREAM_TRACE(err) << STREAM_TRACE(omega) << STREAM_TRACE(rTh) << ")" << std::endl;
		return success;
    }

private:
	// Test if a floating point number is (close to) 0.0
	static inline bool IsZero(CoeffType a) 
	{
		// LS 03/2005: replace Lsolver test by Laspack test to parameterize venus-loop.off w/ authalic/square method
		//#define IsZero(a) (fabs(a) < 1e-40)
		return (fabs(a) < 10.0 * numeric_limits<CoeffType>::min());
	}

private:
    CoeffType epsilon_ ;
    unsigned int max_iter_ ;
} ;


}; // namespace OpenNL

#endif 

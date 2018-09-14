/****************************************************************************/
/* This file is part of FreeFem++.                                          */
/*                                                                          */
/* FreeFem++ is free software: you can redistribute it and/or modify        */
/* it under the terms of the GNU Lesser General Public License as           */
/* published by the Free Software Foundation, either version 3 of           */
/* the License, or (at your option) any later version.                      */
/*                                                                          */
/* FreeFem++ is distributed in the hope that it will be useful,             */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            */
/* GNU Lesser General Public License for more details.                      */
/*                                                                          */
/* You should have received a copy of the GNU Lesser General Public License */
/* along with FreeFem++. If not, see <http://www.gnu.org/licenses/>.        */
/****************************************************************************/
// SUMMARY : ...
// LICENSE : LGPLv3
// ORG     : LJLL Universite Pierre et Marie Curie, Paris, FRANCE
// AUTHORS : ...
// E-MAIL  : ...


// *INDENT-OFF* //
//ff-c++-LIBRARY-dep:
//ff-c++-cpp-dep:
// *INDENT-ON* //

#include "ff++.hpp"
#include "AFunction_ext.hpp"
#include <vector>

MatriceMorse<R> * removeHalf(MatriceMorse<R> & A,int sup,bool sym)
{
    int nnz =0;
    int n = A.n;
    
    if( A.symetrique )
        return new MatriceMorse<R>(n,n,A.nbcoef,sym,A.a,A.lg,A.cl,true);//  copy
    KN<int> ll(n+1);
    ll=0;
    for(int i=0; i< n; ++i)
    {
        
        for(int k=A.lg[i]; k< A.lg[i+1]; ++k)
        {
            int j =A.cl[k] ;
            ffassert(j>=0 && j < n);
            if      ( sup>0 && j  >= i) ll[j+1]++,nnz++;
            else if ( sup<=0 && j <= i) ll[i+1]++,nnz++;
        }
    }
    // do alloc
    MatriceMorse<R> *r=new MatriceMorse<R>(n,n,nnz,sym);
    if(verbosity>1)
        cout << " nnz = "<< nnz << " sup=" << sup << " sym = "<< sym << endl;
    int *cl =r->cl, *lg = r->lg;
    double *a =r->a;
    for(int i=0; i< n; ++i)
        ll[i+1] += ll[i];
     for(int i=0; i<= n; ++i)
         lg[i]=ll[i];
    ffassert(ll[n]== nnz);
    for(int i=0; i< n; ++i)
    {
        //cout << i << " " << lg[i] << " :: " ;
        for(int k=A.lg[i]; k< A.lg[i+1]; ++k)
        {
            int j =A.cl[k] ;
            double aij = A.a[k];
            int kk=-1,ij=-1;
            if(sup>0 && j >=i ) kk = ll[j]++,ij=i;
            else if (sup<=0 && j <=i) kk = ll[i]++,ij=j;
            if(kk>=0)
                cl[kk]=ij, a[kk]=aij;
            //if(kk>=0) cout << kk  << " " << j  << " / " << ij << " , "  ;
        }
        //cout << endl;
    }
    
    lg[n]=nnz;
    ffassert(ll[n]== nnz);
    return r;
}

Matrice_Creuse<R> *removeHalf(Stack stack,Matrice_Creuse<R> *const & pA,long const & sup,bool const & sym)
{
    MatriceMorse<R> *pma=pA->A->toMatriceMorse(false,false);
    Matrice_Creuse<R> *Mat= new Matrice_Creuse<R> ;
    Mat->A.master(removeHalf(*pma,sup,sym));
  //  Add2StackOfPtr2Free(stack,Mat);
    return Mat;
}
Matrice_Creuse<R> *removeHalf(Stack stack,Matrice_Creuse<R> *const & pA,long const & sup)
{
    return removeHalf(stack,pA,sup,0);
}
void ichol(MatriceMorse<R> & A,MatriceMorse<R> &  L,double tgv)
{
    // cf https://en.wikipedia.org/wiki/Incomplete_Cholesky_factorization
    cout << " tgv " << tgv << endl;
    ffassert( A.symetrique && L.symetrique);
    ffassert( A.n == L.n);
    int n =A.n,i,j,k,kk;
    double tgve =tgv*0.99999999;
    if(tgve < 1) tgve=1e200;
    double nan=sqrt(-1.);
    for(int k=0; k<L.nbcoef;++k)
        L.a[k]=nan;
    int BC = 0;
    for(int i=0; i< n; ++i)
    {
        int ai1=A.lg[i+1]-1;
        int ai0=A.lg[i];
        int li1=L.lg[i+1]-1;
        int li0=L.lg[i];
        double  Aii=A.a[ai1];
        if (Aii > tgve)
        { // B.C
            for (kk=li0;kk<li1;kk++)
                L.a[kk]=0; //  remove row and col
            L.a[li1]=1.;
            BC++;
        }
        else
        {
            for (kk=li0;kk<li1;kk++) //  Build Lij is existe j<i
            {
                int j = L.cl[kk]; // j < i
                ffassert(j<i);
                int lj1=L.lg[j+1]-1;
                int lj0=L.lg[j];
                
                double *pAij = A.pij(i,j) ;
                double Lij = pAij ? *pAij: 0.,Aij=Lij;
                for(int kkk= lj0; kkk<lj1; ++kkk)// loop  row j
                {  //cout << " ?? " << kkk << " "<< lj0 << " " <<lj1 <<endl;
                    int k = L.cl[kkk];
                    //cout << " @@@" << i << " " << j << " ( " << lj0 << " " << lj1 << ")  " << k << " // " << kkk <<  " " << endl;
                    ffassert(k >=0 && k < j);
                    double Ljk = L.a[kkk], *pLik=L.pij(i,k), Lik = pLik ? *pLik : 0.;
                    //cout << " *** " << k << " " <<Lik *Ljk <<endl;
                    Lij -= Lik *Ljk;
                }
                Lij /=  L(j,j);
                L.a[kk] =Lij;
                //cout <<kk << " " << j << " " << Lij << " "<< Aij << " , ";
                
            }
            // cout << " **" << endl;
            for(int k= li0; k<li1; ++k)
                Aii -= L.a[k]*L.a[k];
            if( Aii <=0.) { cout << i << " " << Aii << " " << A.a[ai1] << endl; Aii=1;}
            double Lii = sqrt(Aii);
            //  cout << " L " << i << " " << Lii << " " << A.a[ai1]  << endl;
            L.a[li1] =Lii;
            
        }
    }
    cout << " N BC = " << BC <<endl;
}

inline R pscal(R*L,int *cl,int kl,int kl1,int i, MatriceMorse<R> &  Ut,int j )
{
    int ku = Ut.lg[j],ku1=Ut.lg[j]-1;
    int k= min(i,j); //  common  part
    R r =0;
    cout << " pscal: "<<  i << " " << j << "  min: " << k << endl;
    for(int l=kl;l<kl1;++l)
    {
        
        int jl = cl[l];
        cout << "     ##" <<i << " " << jl << " " << (jl > k) << endl;
        if( jl >= k) break;
        R Lijl = L[l];
        R * pUtjjl = Ut.pij(j,jl);
        if(pUtjjl) { r += Lijl* *pUtjjl;
            cout <<   "   **  "<< Lijl << " " << *pUtjjl << " " << jl <<  " " << r << endl;}
    }
    ffassert ( r==r );
    return r;
}
void iLU(MatriceMorse<R> & A,MatriceMorse<R> &  L,MatriceMorse<R> &  Ut,double tgv)
{
/*
 L = L + I, U = U+D
 for(int i=0;i<n; ++i)
 {
 for(int j=0;j<i;++j) L(i,j) = (A(i,j) - (L(i,':'),U(':',j)))/ D(j,j);
 for(int j=0;j<i;++j) U(j,i) = (A(j,i) - (L(j,':'),U(':',i))) ;
 D(i,i) = A(i,i) - (L(i,':'),U(':',i));
 }

 */
    cout << " tgv " << tgv << endl;
    ffassert( A.n == L.n);
    ffassert( A.n == Ut.n);
    int n =A.n,i,j,k,kk;
    double tgve =tgv*0.99999999;
    if(tgve < 1) tgve=1e200;
    double NaN=sqrt(-1.);
    fill(L.a,L.a+L.nbcoef,NaN);
    fill(Ut.a,Ut.a+Ut.nbcoef,NaN);
    int BC = 0;
    int err=0;
    for(int i=0; i< n; ++i)
    {
        int ai1=A.lg[i+1]-1;
        int ai0=A.lg[i];
        int li1=L.lg[i+1]-1;
        int li0=L.lg[i];
        int ui1=Ut.lg[i+1]-1;
        int ui0=Ut.lg[i];
        err += Ut.cl[ui1] != i;
        err += L.cl[li1] != i;

        double  Aii=A.a[ai1],Uii;
        if (Aii > tgve)
        { // B.C
            fill(L.a+li0,L.a+li1,0.);
            fill(Ut.a+ui0,Ut.a+ui1,0.);
            L.a[li1]=1.;
            Ut.a[ui1]=1.;
            BC++;
        }
        else
        {
            for(int l=li0;l<li1;++l) // coef of  L non zero
            {
                R j   = L.cl[l];
                R *pAij = A.pij(i,j), Aij = pAij ? *pAij : 0.;
                ffassert(j<i);
                R Uii = Ut(i,i);
                L.a[l] = (Aij - pscal(L.a,L.cl,li0,li1,i, Ut,j)) / Uii;
            }
            for(int u=ui0;u<ui1;++u) // coef of  Ut  non zero
            {
                R j   = Ut.cl[u];// Ut(j,i) == U(j,i)
                R *pAij = A.pij(j,i), Aij = pAij ? *pAij : 0.;
                ffassert(j<i);// transpose
                R Uii = Ut(i,i);
                Ut.a[u] = (Aij - pscal(Ut.a,Ut.cl,ui0,ui1,i, L,j));
            }
        // set Diag term
            Ut(i,i) = Uii= A(i,i) - pscal(Ut.a,Ut.cl,ui0,ui1,i, L,i);
            cout << i << " " << Uii << endl;
            L(i,i) =1.;
            ffassert(abs(Uii)> 1e-30);
        }
    }
    cout << " N BC = " << BC << "nb err =" << err <<endl;
    assert(err==0);
}


bool ff_ilu (Matrice_Creuse<R> * const & pcA,Matrice_Creuse<R> * const & pcL,Matrice_Creuse<R> * const & pcU,double const & tgv)
{
    MatriceCreuse<R> * pa=pcA->A;
    MatriceCreuse<R> * pl=pcL->A;
    MatriceCreuse<R> * pu=pcU->A;
    ffassert( pa  && pl && pu);
    MatriceMorse<R> *pA= dynamic_cast<MatriceMorse<R>* > (pa);
    MatriceMorse<R> *pL = dynamic_cast<MatriceMorse<R>* > (pl);
    MatriceMorse<R> *pU = dynamic_cast<MatriceMorse<R>* > (pu);
    ffassert(pL && pA && pU);
    iLU(*pA,*pL,*pU,tgv);
    return true;
}

bool ff_ichol (Matrice_Creuse<R> * const & pcA,Matrice_Creuse<R> * const & pcL,double const & tgv)
{
    MatriceCreuse<R> * pa=pcA->A;
    MatriceCreuse<R> * pl=pcL->A;
    ffassert( pa  && pl );
    MatriceMorse<R> *pA= dynamic_cast<MatriceMorse<R>* > (pa);
    MatriceMorse<R> *pL = dynamic_cast<MatriceMorse<R>* > (pl);
    ffassert(pL && pA);
    ichol(*pA,*pL,tgv);
    return true;
}
bool ff_ilu (Matrice_Creuse<R> *  const & pcA,Matrice_Creuse<R> *  const & pcL,Matrice_Creuse<R> * const & pcU)
{
    return ff_ilu(pcA,pcL,pcU, ff_tgv);
}
bool ff_ichol (Matrice_Creuse<R> *  pcA,Matrice_Creuse<R> *  pcL)
{
    return ff_ichol(pcA,pcL,ff_tgv);
}
void ichol_solve(MatriceMorse<R> &L,KN<double> & b,bool trans)
{
    int n =L.n,i,j,k,k1,k0;
    ffassert(L.symetrique);
    ffassert( L.n == b.N());
    if(trans)
    {
        for(int i=n-1; i>=0; --i)
        {
            k0 = L.lg[i];
            k1 = L.lg[i+1]-1;
            b[i] /= L.a[k1];
            
            for (k=k0;k<k1;k++)
            {
                int j = L.cl[k];
                b[j] -= b[i]*L.a[k];
            }
            
            assert(L.cl[k] == i);
        }
    }
    else
    {
        for(int i=0; i< n; ++i)
        {
            R bi= b[i];
            for (k=L.lg[i];k<L.lg[i+1]-1;k++)
            {
                int j = L.cl[k];
                bi -= b[j]*L.a[k];
            }
            b[i] = bi/ L.a[k];
            assert(L.cl[k] == i);
        }
        
    }
    
    
    
}
bool ff_ichol_solve(Matrice_Creuse<R> * pcL,KN<double> * b)
{
    // L L' u = b =>  L uu = b;  L' u = uu;
    MatriceCreuse<R> * pl=pcL->A;
    ffassert(pl );
    MatriceMorse<R> *pL = dynamic_cast<MatriceMorse<R>* > (pl);
    ffassert(pL );
    ichol_solve(*pL,*b,0);
    ichol_solve(*pL,*b,1);
    
    return true;
}
bool ff_ilu_solve(Matrice_Creuse<R> * const & pcL,Matrice_Creuse<R> *const &  pcU,KN<double> * const & b)
{
    // L L' u = b =>  L uu = b;  L' u = uu;
    MatriceCreuse<R> * pl=pcL->A;
    ffassert(pl );
    MatriceMorse<R> *pL = dynamic_cast<MatriceMorse<R>* > (pl);
    ffassert(pL );
    MatriceCreuse<R> * pu=pcU->A;
    ffassert(pu );
    MatriceMorse<R> *pU = dynamic_cast<MatriceMorse<R>* > (pu);
    ffassert(pl );
    ichol_solve(*pL,*b,0);
    ichol_solve(*pU,*b,1);
    
    return true;
}


static void Load_Init () {
    cout << " lood: init Incomplete Cholesky " << endl;
    Global.Add("ichol", "(", new OneOperator2<bool,Matrice_Creuse<R> * ,Matrice_Creuse<R> * >(ff_ichol));
    Global.Add("ichol", "(", new OneOperator3_<bool,Matrice_Creuse<R> * ,Matrice_Creuse<R> * ,double >(ff_ichol));
    Global.Add("iLU", "(", new OneOperator4_<bool,Matrice_Creuse<R> * ,Matrice_Creuse<R> * ,Matrice_Creuse<R> *,double >(ff_ilu));
    Global.Add("iLU", "(", new OneOperator3_<bool,Matrice_Creuse<R> * ,Matrice_Creuse<R> * ,Matrice_Creuse<R> * >(ff_ilu));
    Global.Add("iluSolve", "(", new OneOperator3_<bool ,Matrice_Creuse<R> * ,Matrice_Creuse<R> * , KN<R> *>(ff_ilu_solve));
    
    Global.Add("icholSolve", "(", new OneOperator2<bool ,Matrice_Creuse<R> * , KN<R> *>(ff_ichol_solve));
    Global.Add("removeHalf", "(", new OneOperator2s_<Matrice_Creuse<R> * ,Matrice_Creuse<R> * ,long>(removeHalf));
    Global.Add("removeHalf", "(", new OneOperator3s_<Matrice_Creuse<R> * ,Matrice_Creuse<R> * ,long,bool>(removeHalf));

}

LOADFUNC(Load_Init)

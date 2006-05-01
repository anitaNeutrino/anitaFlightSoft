/* utilities for computing coincidence and majority logic */

/* We use unsigned shorts containing on bit for
each sector.  We can then do rotates
to make sector combinations. */

unsigned short rotl(unsigned short x, unsigned short r)
{
     return (x << r) | (x >> (sizeof(x)*8 - r));
}

unsigned short add(int n, unsigned short *a, unsigned short *b, 
		   unsigned short *sum)
{
     /* a, b, and sum are in the representation sum[0]=LSB, 
	sum[1]=twos, sum[3]=fours,... sum[n-1].
     */
     int i;
     unsigned short carry, savea, saveb;
     carry=0;
     for(i=0;i<n;i++){
	  savea=a[i]; saveb=b[i]; /* allows a or b to be sum */
	  sum[i]= (savea ^ saveb) ^ carry;
	  carry= (savea & saveb) | (savea & carry) | (saveb & carry);
     }
     return carry; /* nonzero carry indicates an overflow occurred */
}

unsigned short compare16(int n, unsigned short *sum, unsigned short threshold)
{
     /* returns a bit mask indicating which of the sector bits in the
	sum (see comments in add for representation) are representing
	at a level greater than threshold.  Note this is _strictly_
	greater than, e.g. for threefold threshold==2 
	See T&S 19.4 for equations.
     */
     int i;
     unsigned short result, identity, mask;
     result=0; identity=0xffff;
     for(i=(n-1); i>=0; i--){
	  mask=((threshold >> i) & (unsigned short) 0x1) 
	       + (unsigned short) 0xffff ; 
              /* 0 if bit set, 0xffff if bit clear */
	      /* if this is too slow, we could push 
		 mask building to the caller... */
	  result |= identity & (a[i] & mask);
     }
     return result;
}

unsigned short sectorCoinc(unsigned short threshold, unsigned short m, unsigned short bits)
{
     /* build a sector of width m, and check for coincidence
	at a level greater than 'level
	A coincidence appears in the bit position of the highest
	numbered antenna in the sector.
     */
     if (m>16) m=16;
     unsigned short sum[5]={0,0,0,0,0}, cur_bits;
     for (i=0; i<m; i++){
	  cur_bits=rotl(bits,i};
	  add(5,&bits,sum,sum);
     }
     return compare16(5,sum,threshold);
}
     

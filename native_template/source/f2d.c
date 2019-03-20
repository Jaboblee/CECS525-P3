
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include "f2d.h"

//#define RYU_DEBUG

void toString_hex(uint32_t num, char* numArray) {
    uint32_t temp;
    numArray[0] = '0';
    numArray[1] = 'x';

    for (int i=9;i>=2;i--) {
        temp = 0xF & num;
        num = num >> 4;
        if (temp >=0 && temp <=9) {numArray[i] = temp + 48;}
        else {numArray[i] = temp + 55;}
    }
}

float32 stringToFloat(char* s, int n) {
	uint32_t ipart = 0;				//Integer part
	uint32_t fpart = 0;				//Fractional part
	uint32_t fdigits = 0;
	static uint32_t base[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000};
	uint32_t b_exp = 150;

	uint32_t result = 0;

	int neg = 0;
	int i = 0;

	if (s[0] == 45) {
		neg = 1;
		i = 1;
	}

	while ((i < n) && (s[i] >= 48 && s[i] <= 57)) {

		ipart *= 10;
		ipart += (s[i] - '0');
		i++;
	}

	result = ipart;

	if (s[i] == '.') {
		i++;
		fdigits = i;

		while ((i < n) && (s[i] >= 48 && s[i] <= 57)) {

			fpart *= 10;
			fpart += (s[i] - '0');
			i++;
		}

		fdigits = i - fdigits;
	}

	if (ipart == 0 && fpart == 0) {return 0;}

	while (result & 0xFF000000) {
		result = (result >> 1);
		b_exp++;
	}

	while (!(result & 0x800000)) {
		result = (result << 1);
		if (fdigits != 0) {
			fpart = (fpart << 1);
			if ((fpart / base[fdigits]) % 10) {
				result++;
				fpart = fpart - (((fpart / base[fdigits]) % 10)*base[fdigits]);
			}
		}
		b_exp--;
	}

	result = ((result & 0x807FFFFF) | (b_exp << 23));
	if (neg != 0) {result = (result | 0x80000000);}

	return result;
}

// This table is generated by PrintFloatLookupTable.
#define FLOAT_POW5_INV_BITCOUNT 59
static const uint64_t FLOAT_POW5_INV_SPLIT[31] = {
  576460752303423489u, 461168601842738791u, 368934881474191033u, 295147905179352826u,
  472236648286964522u, 377789318629571618u, 302231454903657294u, 483570327845851670u,
  386856262276681336u, 309485009821345069u, 495176015714152110u, 396140812571321688u,
  316912650057057351u, 507060240091291761u, 405648192073033409u, 324518553658426727u,
  519229685853482763u, 415383748682786211u, 332306998946228969u, 531691198313966350u,
  425352958651173080u, 340282366920938464u, 544451787073501542u, 435561429658801234u,
  348449143727040987u, 557518629963265579u, 446014903970612463u, 356811923176489971u,
  570899077082383953u, 456719261665907162u, 365375409332725730u
};
#define FLOAT_POW5_BITCOUNT 61
static const uint64_t FLOAT_POW5_SPLIT[47] = {
  1152921504606846976u, 1441151880758558720u, 1801439850948198400u, 2251799813685248000u,
  1407374883553280000u, 1759218604441600000u, 2199023255552000000u, 1374389534720000000u,
  1717986918400000000u, 2147483648000000000u, 1342177280000000000u, 1677721600000000000u,
  2097152000000000000u, 1310720000000000000u, 1638400000000000000u, 2048000000000000000u,
  1280000000000000000u, 1600000000000000000u, 2000000000000000000u, 1250000000000000000u,
  1562500000000000000u, 1953125000000000000u, 1220703125000000000u, 1525878906250000000u,
  1907348632812500000u, 1192092895507812500u, 1490116119384765625u, 1862645149230957031u,
  1164153218269348144u, 1455191522836685180u, 1818989403545856475u, 2273736754432320594u,
  1421085471520200371u, 1776356839400250464u, 2220446049250313080u, 1387778780781445675u,
  1734723475976807094u, 2168404344971008868u, 1355252715606880542u, 1694065894508600678u,
  2117582368135750847u, 1323488980084844279u, 1654361225106055349u, 2067951531382569187u,
  1292469707114105741u, 1615587133892632177u, 2019483917365790221u
};

static inline uint32_t pow5Factor(uint32_t value) {
  uint32_t count = 0;
  for (;;) {
    assert(value != 0);
    const uint32_t q = value / 5;
    const uint32_t r = value % 5;
    if (r != 0) {
      break;
    }
    value = q;
    ++count;
  }
  return count;
}

// Returns true if value is divisible by 5^p.
static inline bool multipleOfPowerOf5(const uint32_t value, const uint32_t p) {
  return pow5Factor(value) >= p;
}

// Returns true if value is divisible by 2^p.
static inline bool multipleOfPowerOf2(const uint32_t value, const uint32_t p) {
  // return __builtin_ctz(value) >= p;
  return (value & ((1u << p) - 1)) == 0;
}

// It seems to be slightly faster to avoid uint128_t here, although the
// generated code for uint128_t looks slightly nicer.
static inline uint32_t mulShift(const uint32_t m, const uint64_t factor, const int32_t shift) {
  assert(shift > 32);

  // The casts here help MSVC to avoid calls to the __allmul library
  // function.
  const uint32_t factorLo = (uint32_t)(factor);
  const uint32_t factorHi = (uint32_t)(factor >> 32);
  const uint64_t bits0 = (uint64_t)m * factorLo;
  const uint64_t bits1 = (uint64_t)m * factorHi;

  // On 32-bit platforms we can avoid a 64-bit shift-right since we only
  // need the upper 32 bits of the result and the shift value is > 32.
  const uint32_t bits0Hi = (uint32_t)(bits0 >> 32);
  uint32_t bits1Lo = (uint32_t)(bits1);
  uint32_t bits1Hi = (uint32_t)(bits1 >> 32);
  bits1Lo += bits0Hi;
  bits1Hi += (bits1Lo < bits0Hi);
  const int32_t s = shift - 32;
  return (bits1Hi << (32 - s)) | (bits1Lo >> s);
}

static inline uint32_t mulPow5InvDivPow2(const uint32_t m, const uint32_t q, const int32_t j) {
  return mulShift(m, FLOAT_POW5_INV_SPLIT[q], j);
}

static inline uint32_t mulPow5divPow2(const uint32_t m, const uint32_t i, const int32_t j) {
  return mulShift(m, FLOAT_POW5_SPLIT[i], j);
}

// Returns e == 0 ? 1 : ceil(log_2(5^e)).
static inline int32_t pow5bits(const int32_t e) {
  // This approximation works up to the point that the multiplication overflows at e = 3529.
  // If the multiplication were done in 64 bits, it would fail at 5^4004 which is just greater
  // than 2^9297.
  assert(e >= 0);
  assert(e <= 3528);
  return (int32_t) (((((uint32_t) e) * 1217359) >> 19) + 1);
}

// Returns floor(log_10(2^e)).
static inline uint32_t log10Pow2(const int32_t e) {
  // The first value this approximation fails for is 2^1651 which is just greater than 10^297.
  assert(e >= 0);
  assert(e <= 1650);
  return (((uint32_t) e) * 78913) >> 18;
}

// Returns floor(log_10(5^e)).
static inline uint32_t log10Pow5(const int32_t e) {
  // The first value this approximation fails for is 5^2621 which is just greater than 10^1832.
  assert(e >= 0);
  assert(e <= 2620);
  return (((uint32_t) e) * 732923) >> 20;
}

float_d f2d(float32 f) {

    float_d fd;
    uint32_t m;
    int32_t e;

    uint32_t u,v,w;
    uint32_t a,b,c;
    int32_t e2, e10;

    fd.s = (f & 0x80000000);

    e = (f & 0x7F800000) >> 23;
    m = (f & 0x007FFFFF);
    bool ushift = (m != 0) || (e <= 1);

    //Check NaN, +-0, +-inf

    if (e == 0) {
        e2 = -149 - 2 ;
    } else {
        e2 = e - 150 - 2;
        m = m | 0x00800000;
    }

    bool acceptBounds = ((m & 1) == 0); //Even

    u = (m << 2) - 1 - ushift;  //m
    v = (m << 2);               //v
    w = (m << 2) + 2;           //p

    //step 3
    bool aIsTrailingZeros = false;
    bool bIsTrailingZeros = false;
    uint8_t lastRemovedDigit = 0;

    if (e2 >= 0) {
        const uint32_t q = log10Pow2(e2);
        e10 = (int32_t) q;
        const int32_t k = FLOAT_POW5_INV_BITCOUNT + pow5bits((int32_t) q) - 1;
        const int32_t i = -e2 + (int32_t) q + k;
        b = mulPow5InvDivPow2(v, q, i);
        c = mulPow5InvDivPow2(w, q, i);
        a = mulPow5InvDivPow2(u, q, i);
    #ifdef RYU_DEBUG
        printf("%u * 2^%d / 10^%u\n", v, e2, q);
        printf("V+=%u\nV =%u\nV-=%u\n", c, b, a);
    #endif
        if (q != 0 && (c - 1) / 10 <= a / 10) {
          // We need to know one removed digit even if we are not going to loop below. We could use
          // q = X - 1 above, except that would require 33 bits for the result, and we've found that
          // 32-bit arithmetic is faster even on 64-bit machines.
          const int32_t l = FLOAT_POW5_INV_BITCOUNT + pow5bits((int32_t) (q - 1)) - 1;
          lastRemovedDigit = (uint8_t) (mulPow5InvDivPow2(v, q - 1, -e2 + (int32_t) q - 1 + l) % 10);
        }
        if (q <= 9) {
          // The largest power of 5 that fits in 24 bits is 5^10, but q <= 9 seems to be safe as well.
          // Only one of mp, mv, and mm can be a multiple of 5, if any.
          if (v % 5 == 0) {
            bIsTrailingZeros = multipleOfPowerOf5(v, q);
          } else if (acceptBounds) {
            aIsTrailingZeros = multipleOfPowerOf5(u, q);
          } else {
            c -= multipleOfPowerOf5(w, q);
          }
        }
      } else {
        const uint32_t q = log10Pow5(-e2);
        e10 = (int32_t) q + e2;
        const int32_t i = -e10; //-e2 - (int32_t) q;
        const int32_t k = pow5bits(i) - FLOAT_POW5_BITCOUNT;
        int32_t j = (int32_t) q - k;
        b = mulPow5divPow2(v, (uint32_t) i, j);
        c = mulPow5divPow2(w, (uint32_t) i, j);
        a = mulPow5divPow2(u, (uint32_t) i, j);
    #ifdef RYU_DEBUG
        printf("%u * 5^%d / 10^%u\n", v, -e2, q);
        printf("%u %d %d %d\n", q, i, k, j);
        printf("V+=%u\nV =%u\nV-=%u\n", c, b, a);
    #endif
        if (q != 0 && (c - 1) / 10 <= a / 10) {
          j = (int32_t) q - 1 - (pow5bits(i + 1) - FLOAT_POW5_BITCOUNT);
          lastRemovedDigit = (uint8_t) (mulPow5divPow2(v, (uint32_t) (i + 1), j) % 10);
        }
        if (q <= 1) {
          // {vr,c,a} is trailing zeros if {mv,mp,mm} has at least q trailing 0 bits.
          // mv = 4 * m2, so it always has at least two trailing 0 bits.
          bIsTrailingZeros = true;
          if (acceptBounds) {
            // mm = mv - 1 - mmShift, so it has 1 trailing 0 bit iff mmShift == 1.
            aIsTrailingZeros = ushift == 1;
          } else {
            // mp = mv + 2, so it always has at least one trailing 0 bit.
            --c;
          }
        } else if (q < 31) { // TODO(ulfjack): Use a tighter bound here.
          bIsTrailingZeros = multipleOfPowerOf2(v, q - 1);
    #ifdef RYU_DEBUG
          printf("vr is trailing zeros=%s\n", bIsTrailingZeros ? "true" : "false");
    #endif
        }
      }

  #ifdef RYU_DEBUG
  printf("e10=%d\n", e10);
  printf("V+=%u\nV =%u\nV-=%u\n", c, b, a);
  printf("a is trailing zeros=%s\n", aIsTrailingZeros ? "true" : "false");
  printf("vr is trailing zeros=%s\n", bIsTrailingZeros ? "true" : "false");
  #endif

  // Step 4: Find the shortest decimal representation in the interval of valid representations.
  int32_t removed = 0;
  uint32_t output;
  if (aIsTrailingZeros || bIsTrailingZeros) {
    // General case, which happens rarely (~4.0%).
    while (c / 10 > a / 10) {

      aIsTrailingZeros &= a % 10 == 0;
      bIsTrailingZeros &= lastRemovedDigit == 0;
      lastRemovedDigit = (uint8_t) (b % 10);
      b /= 10;
      c /= 10;
      a /= 10;
      ++removed;
    }
#ifdef RYU_DEBUG
    printf("V+=%u\nV =%u\nV-=%u\n", c, b, a);
    printf("d-10=%s\n", aIsTrailingZeros ? "true" : "false");
#endif
    if (aIsTrailingZeros) {
      while (a % 10 == 0) {
        bIsTrailingZeros &= lastRemovedDigit == 0;
        lastRemovedDigit = (uint8_t) (b % 10);
        b /= 10;
        c /= 10;
        a /= 10;
        ++removed;
      }
    }
#ifdef RYU_DEBUG
    printf("%u %d\n", b, lastRemovedDigit);
    printf("vr is trailing zeros=%s\n", bIsTrailingZeros ? "true" : "false");
#endif
    if (bIsTrailingZeros && lastRemovedDigit == 5 && b % 2 == 0) {
      // Round even if the exact number is .....50..0.
      lastRemovedDigit = 4;
    }
    // We need to take b + 1 if b is outside bounds or we need to round up.
    output = b + ((b == a && (!acceptBounds || !aIsTrailingZeros)) || lastRemovedDigit >= 5);
  } else {
    // Specialized for the common case (~96.0%). Percentages below are relative to this.
    // Loop iterations below (approximately):
    // 0: 13.6%, 1: 70.7%, 2: 14.1%, 3: 1.39%, 4: 0.14%, 5+: 0.01%
    while (c / 10 > a / 10) {
      lastRemovedDigit = (uint8_t) (b % 10);
      b /= 10;
      c /= 10;
      a /= 10;
      ++removed;
    }
#ifdef RYU_DEBUG
    printf("%u %d\n", b, lastRemovedDigit);
    printf("b is trailing zeros=%s\n", bIsTrailingZeros ? "true" : "false");
#endif
    // We need to take b + 1 if b is outside bounds or we need to round up.
    output = b + (b == a || lastRemovedDigit >= 5);
  }
  const int32_t exp = e10 + removed;

#ifdef RYU_DEBUG
  printf("V+=%u\nV =%u\nV-=%u\n", c, b, a);
  printf("O=%u\n", output);
  printf("EXP=%d\n", exp);
#endif

    fd.e = exp;
    fd.m = output;

    return fd;
}

int floatToString(float32 f, char* s, int n) {
/*
    uint32_t ipart = 0;				//Integer part
	uint32_t fpart = 0;				//Fractional part
    uint32_t b_exp = 150;

    union i_f {
        float f;
        uint32_t i;
	} i_f;

	i_f.f = f;

	int j = 0;
	int neg = 0;

    if (i_f.i & 0x007FFFFF == 0) {
        s[0] = '0';
        s[1] = '\0';
        return 0;
    }

    if (i_f.i & 0x80000000) {
        neg = 1;
        s[j] = '-';
        j++;
    }

    b_exp = ((i_f.i & 0x7F800000) >> 23);

    if (b_exp == 127) {
        b_exp = 0;
        ipart = 1;
        fpart = (i_f.i & 0x007FFFFF);
    } else if (b_exp < 127) {
        b_exp = 127 - b_exp;
        ipart = 0;
        fpart = 0x00800000;
        fpart = (i_f.i & 0x007FFFFF);
    } else if (b_exp > 127 && b_exp < 150){
        b_exp = b_exp - 127;
        uint32_t temp = 0;
        for (int i=0;i<b_exp;i++) {
            temp = (temp >> 1) + 0x00400000;
        }
        ipart = (0x00800000 | (i_f.i & temp)) >> (23 - b_exp);
        fpart = ~(0xFFE00000 | temp) & i_f.i;
    } else if (b_exp >= 150) {
        b_exp = b_exp - 127;
        fpart = 0;
        ipart = 0x00800000;
        ipart = (i_f.i & 0x007FFFFF);
    }
*/

//printf("%f\n", i_f.f);

    int accept_low = 1;
    int accept_high = 1;
    int break_tie_down = 1;

    int i = 0;
    uint32_t m;
    int32_t e;

    uint32_t u;
    uint32_t v;
    uint32_t w;
    int32_t e2;

    /*
    if ((f & 0x007FFFFF) == 0) {
        s[0] = '0';
        s[1] = '\0';
        return 0;
    }*/

    if (f & 0x80000000) {
        s[i]='-';
        i++;
    }

    e = (f & 0x7F800000) >> 23;
//printf("%d\n", e);
    if (e == 0) {
        e = -149;
        m = (f & 0x007FFFFF);
    } else {
        e = e - 150;
        m = (f & 0x007FFFFF) | 0x00800000;
    }

    e2 = e - 2;

    if ((f & 0x007FFFFF) == 0 && ((f & 0x7F800000) >> 23) > 1) {
        u = (m << 2) - 1;
    } else {
        u = (m << 2) - 2;
    }
    v = (m << 2);
    w = (m << 2) + 2;

//printf("%u , %u, %u\n", u, v, w);

    uint32_t a;
    uint32_t b;
    uint32_t c;
    int32_t e10;

    if (e2 >= 0) {
        e10 = 0;
        a = (u << e2);
        b = (v << e2);
        c = (w << e2);
    } else {
        e10 = e2;

        uint32_t temp = 1;
        for (int j=0;j>e10;j--) {       //too large!
            temp = temp * 5;
        }

//printf("%u\n",temp);
        a = u * temp;
        b = v * temp;
        c = w * temp;
    }

//printf("%u, %u, %u\n", a, b, c);

    int k = 0;
    int all_a_zero = 1;
    int all_b_zero = 1;
    int digit = 0;

    if (!(accept_high)) {c = c - 1;}

    while ((a/10) < (c/10)) {
        all_a_zero = all_a_zero & (a%10 == 0);
        all_b_zero = all_b_zero & (digit == 0);
        digit = b % 10;
        a = a / 10;
        b = b / 10;
        c = c / 10;
        k++;
    }
    if (accept_low && all_a_zero) {
        while (a%10 == 0) {
            all_b_zero = all_b_zero & (digit == 0);
            digit = b % 10;
            a = a / 10;
            b = b / 10;
            c = c / 10;
            k++;
        }
    }
    int is_tie = ((digit == 5) && (all_b_zero));
    int want_round_down = (digit < 5) | (is_tie & break_tie_down);
    int round_down = (want_round_down & ((a != b) | all_a_zero)) | ((b + 1) > c);

    if (!(round_down)) {b = b +1;}

//    printf("\t%ue%d\n", b, k + e10);

    //printf("\t%d\n", ipart);

    return 0;
}

/*
uint32_t pow(uint32_t a, uint32_t b) {
    uint32_t out = 1;

    if (b = 0) {return 1;}
    while (b > 0) {
        out = out * a;
        b--;
    }
    return out;
}*/

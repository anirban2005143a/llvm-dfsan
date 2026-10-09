int type;
int nb;
int round_val;
int key[32];
int statemt[32];
int word[4][120];

/* key generate */
int KeySchedule(int, int *);
int SubByte(int);

void ByteSub_ShiftRow(int *, int);
int MixColumn_AddRoundKey(int *, int, int);
int AddRoundKey(int *, int, int);
int encrypt(int *, int *, int);
int main_result;

////////////////////
int encrypt(int statemt[32], int key[32], int type) {
  int i;
  /*
  +--------------------------------------------------------------------------+
  | * Test Vector (added for CHStone)                                        |
  |     out_enc_statemt : expected output data for "encrypt"                 |
  +--------------------------------------------------------------------------+
  */
  const int out_enc_statemt[16] = {0x39, 0x25, 0x84, 0x1d, 0x2,  0xdc,
                                   0x9,  0xfb, 0xdc, 0x11, 0x85, 0x97,
                                   0x19, 0x6a, 0xb,  0x32};

  KeySchedule(type, key);
  switch (type) {
  case 128128:
    round_val = 0;
    nb = 4;
    break;
  case 192128:
    round_val = 2;
    nb = 4;
    break;
  case 256128:
    round_val = 4;
    nb = 4;
    break;
  case 128192:
  case 192192:
    round_val = 2;
    nb = 6;
    break;
  case 256192:
    round_val = 4;
    nb = 6;
    break;
  case 128256:
  case 192256:
  case 256256:
    round_val = 4;
    nb = 8;
    break;
  }
  AddRoundKey(statemt, type, 0);
  for (i = 1; i <= round_val + 9; ++i) {
    ByteSub_ShiftRow(statemt, nb);
    MixColumn_AddRoundKey(statemt, nb, i);
  }
  ByteSub_ShiftRow(statemt, nb);
  AddRoundKey(statemt, type, i);

  for (i = 0; i < 16; i++) {
    main_result += (statemt[i] != out_enc_statemt[i]);
  }

  return 0;
}
const int Sbox[16][16];

/* ********* ByteSub & ShiftRow ********* */
void ByteSub_ShiftRow(int statemt[32], int nb) {
  int temp;

  switch (nb) {
  case 4:
    temp = Sbox[statemt[1] >> 4][statemt[1] & 0xf];
    statemt[1] = Sbox[statemt[5] >> 4][statemt[5] & 0xf];
    statemt[5] = Sbox[statemt[9] >> 4][statemt[9] & 0xf];
    statemt[9] = Sbox[statemt[13] >> 4][statemt[13] & 0xf];
    statemt[13] = temp;

    temp = Sbox[statemt[2] >> 4][statemt[2] & 0xf];
    statemt[2] = Sbox[statemt[10] >> 4][statemt[10] & 0xf];
    statemt[10] = temp;
    temp = Sbox[statemt[6] >> 4][statemt[6] & 0xf];
    statemt[6] = Sbox[statemt[14] >> 4][statemt[14] & 0xf];
    statemt[14] = temp;

    temp = Sbox[statemt[3] >> 4][statemt[3] & 0xf];
    statemt[3] = Sbox[statemt[15] >> 4][statemt[15] & 0xf];
    statemt[15] = Sbox[statemt[11] >> 4][statemt[11] & 0xf];
    statemt[11] = Sbox[statemt[7] >> 4][statemt[7] & 0xf];
    statemt[7] = temp;

    statemt[0] = Sbox[statemt[0] >> 4][statemt[0] & 0xf];
    statemt[4] = Sbox[statemt[4] >> 4][statemt[4] & 0xf];
    statemt[8] = Sbox[statemt[8] >> 4][statemt[8] & 0xf];
    statemt[12] = Sbox[statemt[12] >> 4][statemt[12] & 0xf];
    break;
  case 6:
    temp = Sbox[statemt[1] >> 4][statemt[1] & 0xf];
    statemt[1] = Sbox[statemt[5] >> 4][statemt[5] & 0xf];
    statemt[5] = Sbox[statemt[9] >> 4][statemt[9] & 0xf];
    statemt[9] = Sbox[statemt[13] >> 4][statemt[13] & 0xf];
    statemt[13] = Sbox[statemt[17] >> 4][statemt[17] & 0xf];
    statemt[17] = Sbox[statemt[21] >> 4][statemt[21] & 0xf];
    statemt[21] = temp;

    temp = Sbox[statemt[2] >> 4][statemt[2] & 0xf];
    statemt[2] = Sbox[statemt[10] >> 4][statemt[10] & 0xf];
    statemt[10] = Sbox[statemt[18] >> 4][statemt[18] & 0xf];
    statemt[18] = temp;
    temp = Sbox[statemt[6] >> 4][statemt[6] & 0xf];
    statemt[6] = Sbox[statemt[14] >> 4][statemt[14] & 0xf];
    statemt[14] = Sbox[statemt[22] >> 4][statemt[22] & 0xf];
    statemt[22] = temp;

    temp = Sbox[statemt[3] >> 4][statemt[3] & 0xf];
    statemt[3] = Sbox[statemt[15] >> 4][statemt[15] & 0xf];
    statemt[15] = temp;
    temp = Sbox[statemt[7] >> 4][statemt[7] & 0xf];
    statemt[7] = Sbox[statemt[19] >> 4][statemt[19] & 0xf];
    statemt[19] = temp;
    temp = Sbox[statemt[11] >> 4][statemt[11] & 0xf];
    statemt[11] = Sbox[statemt[23] >> 4][statemt[23] & 0xf];
    statemt[23] = temp;

    statemt[0] = Sbox[statemt[0] >> 4][statemt[0] & 0xf];
    statemt[4] = Sbox[statemt[4] >> 4][statemt[4] & 0xf];
    statemt[8] = Sbox[statemt[8] >> 4][statemt[8] & 0xf];
    statemt[12] = Sbox[statemt[12] >> 4][statemt[12] & 0xf];
    statemt[16] = Sbox[statemt[16] >> 4][statemt[16] & 0xf];
    statemt[20] = Sbox[statemt[20] >> 4][statemt[20] & 0xf];
    break;
  case 8:
    temp = Sbox[statemt[1] >> 4][statemt[1] & 0xf];
    statemt[1] = Sbox[statemt[5] >> 4][statemt[5] & 0xf];
    statemt[5] = Sbox[statemt[9] >> 4][statemt[9] & 0xf];
    statemt[9] = Sbox[statemt[13] >> 4][statemt[13] & 0xf];
    statemt[13] = Sbox[statemt[17] >> 4][statemt[17] & 0xf];
    statemt[17] = Sbox[statemt[21] >> 4][statemt[21] & 0xf];
    statemt[21] = Sbox[statemt[25] >> 4][statemt[25] & 0xf];
    statemt[25] = Sbox[statemt[29] >> 4][statemt[29] & 0xf];
    statemt[29] = temp;

    temp = Sbox[statemt[2] >> 4][statemt[2] & 0xf];
    statemt[2] = Sbox[statemt[14] >> 4][statemt[14] & 0xf];
    statemt[14] = Sbox[statemt[26] >> 4][statemt[26] & 0xf];
    statemt[26] = Sbox[statemt[6] >> 4][statemt[6] & 0xf];
    statemt[6] = Sbox[statemt[18] >> 4][statemt[18] & 0xf];
    statemt[18] = Sbox[statemt[30] >> 4][statemt[30] & 0xf];
    statemt[30] = Sbox[statemt[10] >> 4][statemt[10] & 0xf];
    statemt[10] = Sbox[statemt[22] >> 4][statemt[22] & 0xf];
    statemt[22] = temp;

    temp = Sbox[statemt[3] >> 4][statemt[3] & 0xf];
    statemt[3] = Sbox[statemt[19] >> 4][statemt[19] & 0xf];
    statemt[19] = temp;
    temp = Sbox[statemt[7] >> 4][statemt[7] & 0xf];
    statemt[7] = Sbox[statemt[23] >> 4][statemt[23] & 0xf];
    statemt[23] = temp;
    temp = Sbox[statemt[11] >> 4][statemt[11] & 0xf];
    statemt[11] = Sbox[statemt[27] >> 4][statemt[27] & 0xf];
    statemt[27] = temp;
    temp = Sbox[statemt[15] >> 4][statemt[15] & 0xf];
    statemt[15] = Sbox[statemt[31] >> 4][statemt[31] & 0xf];
    statemt[31] = temp;

    statemt[0] = Sbox[statemt[0] >> 4][statemt[0] & 0xf];
    statemt[4] = Sbox[statemt[4] >> 4][statemt[4] & 0xf];
    statemt[8] = Sbox[statemt[8] >> 4][statemt[8] & 0xf];
    statemt[12] = Sbox[statemt[12] >> 4][statemt[12] & 0xf];
    statemt[16] = Sbox[statemt[16] >> 4][statemt[16] & 0xf];
    statemt[20] = Sbox[statemt[20] >> 4][statemt[20] & 0xf];
    statemt[24] = Sbox[statemt[24] >> 4][statemt[24] & 0xf];
    statemt[28] = Sbox[statemt[28] >> 4][statemt[28] & 0xf];
    break;
  }
}

int SubByte(int in) { return Sbox[(in / 16)][(in % 16)]; }


/* ******** MixColumn ********** */
int MixColumn_AddRoundKey(int statemt[32], int nb, int n) {
  int ret[8 * 4], j;
  register int x;

  for (j = 0; j < nb; ++j) {
    ret[j * 4] = (statemt[j * 4] << 1);
    if ((ret[j * 4] >> 8) == 1) {
      ret[j * 4] ^= 283;
    }
    x = statemt[1 + j * 4];
    x ^= (x << 1);
    if ((x >> 8) == 1) {
      ret[j * 4] ^= (x ^ 283);
    } else {
      ret[j * 4] ^= x;
    }
    ret[j * 4] ^= statemt[2 + j * 4] ^ statemt[3 + j * 4] ^ word[0][j + nb * n];

    ret[1 + j * 4] = (statemt[1 + j * 4] << 1);
    if ((ret[1 + j * 4] >> 8) == 1) {
      ret[1 + j * 4] ^= 283;
    }
    x = statemt[2 + j * 4];
    x ^= (x << 1);
    if ((x >> 8) == 1) {
      ret[1 + j * 4] ^= (x ^ 283);
    } else {
      ret[1 + j * 4] ^= x;
    }
    ret[1 + j * 4] ^= statemt[3 + j * 4] ^ statemt[j * 4] ^ word[1][j + nb * n];

    ret[2 + j * 4] = (statemt[2 + j * 4] << 1);
    if ((ret[2 + j * 4] >> 8) == 1) {
      ret[2 + j * 4] ^= 283;
    }
    x = statemt[3 + j * 4];
    x ^= (x << 1);
    if ((x >> 8) == 1) {
      ret[2 + j * 4] ^= (x ^ 283);
    } else {
      ret[2 + j * 4] ^= x;
    }
    ret[2 + j * 4] ^= statemt[j * 4] ^ statemt[1 + j * 4] ^ word[2][j + nb * n];

    ret[3 + j * 4] = (statemt[3 + j * 4] << 1);
    if ((ret[3 + j * 4] >> 8) == 1) {
      ret[3 + j * 4] ^= 283;
    }
    x = statemt[j * 4];
    x ^= (x << 1);
    if ((x >> 8) == 1) {
      ret[3 + j * 4] ^= (x ^ 283);
    } else {
      ret[3 + j * 4] ^= x;
    }
    ret[3 + j * 4] ^=
        statemt[1 + j * 4] ^ statemt[2 + j * 4] ^ word[3][j + nb * n];
  }
  for (j = 0; j < nb; ++j) {
    statemt[j * 4] = ret[j * 4];
    statemt[1 + j * 4] = ret[1 + j * 4];
    statemt[2 + j * 4] = ret[2 + j * 4];
    statemt[3 + j * 4] = ret[3 + j * 4];
  }
  return 0;
}



/* ******** AddRoundKey ********** */
int AddRoundKey(int statemt[32], int type, int n) {
  int j, nb;

  switch (type) {
  case 128128:
  case 192128:
  case 256128:
    nb = 4;
    break;
  case 128192:
  case 192192:
  case 256192:
    nb = 6;
    break;
  case 128256:
  case 192256:
  case 256256:
    nb = 8;
    break;
  }
  for (j = 0; j < nb; ++j) {
    statemt[j * 4] ^= word[0][j + nb * n];
    statemt[1 + j * 4] ^= word[1][j + nb * n];
    statemt[2 + j * 4] ^= word[2][j + nb * n];
    statemt[3 + j * 4] ^= word[3][j + nb * n];
  }
  return 0;
}
//////////////////////////////////////////////////

const int Rcon0[30] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36,
    0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6,
    0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91,
};

/*  **************** key expand ************************ */
int KeySchedule(int type, int key[32]) {
  int nk, nb, round_val;
  int i, j, temp[4];

  switch (type) {
  case 128128:
    nk = 4;
    nb = 4;
    round_val = 10;
    break;
  case 128192:
    nk = 4;
    nb = 6;
    round_val = 12;
    break;
  case 128256:
    nk = 4;
    nb = 8;
    round_val = 14;
    break;
  case 192128:
    nk = 6;
    nb = 4;
    round_val = 12;
    break;
  case 192192:
    nk = 6;
    nb = 6;
    round_val = 12;
    break;
  case 192256:
    nk = 6;
    nb = 8;
    round_val = 14;
    break;
  case 256128:
    nk = 8;
    nb = 4;
    round_val = 14;
    break;
  case 256192:
    nk = 8;
    nb = 6;
    round_val = 14;
    break;
  case 256256:
    nk = 8;
    nb = 8;
    round_val = 14;
    break;
  default:
    return -1;
  }
  for (j = 0; j < nk; ++j) {
    for (i = 0; i < 4; ++i) {
      /* 0 word */
      word[i][j] = key[i + j * 4];
    }
  }

  /* expanded key is generated */
  for (j = nk; j < nb * (round_val + 1); ++j) {

    /* RotByte */
    if ((j % nk) == 0) {
      temp[0] = SubByte(word[1][j - 1]) ^ Rcon0[(j / nk) - 1];
      temp[1] = SubByte(word[2][j - 1]);
      temp[2] = SubByte(word[3][j - 1]);
      temp[3] = SubByte(word[0][j - 1]);
    }
    if ((j % nk) != 0) {
      temp[0] = word[0][j - 1];
      temp[1] = word[1][j - 1];
      temp[2] = word[2][j - 1];
      temp[3] = word[3][j - 1];
    }
    if (nk > 6 && j % nk == 4) {
      for (i = 0; i < 4; ++i) {
        temp[i] = SubByte(temp[i]);
      }
    }
    for (i = 0; i < 4; ++i) {
      word[i][j] = word[i][j - nk] ^ temp[i];
    }
  }
  return 0;
}
///////////////////////////////////////////////////

int main() {
  main_result = 0;
  encrypt(statemt, key, 128128);
  return main_result;
}


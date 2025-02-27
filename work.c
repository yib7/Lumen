// 1a.

int cb[8][8];
for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
        cb[r][c] = (r + c) % 2;
    }
}


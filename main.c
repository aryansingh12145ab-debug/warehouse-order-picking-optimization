// Warehouse Order Picking Optimization (Batching + Routing)
// Basic C: arrays, 2D arrays, functions, pointers, malloc/free, sorting.

#include <stdio.h>    // for printf and scanf
#include <stdlib.h>   // for malloc, free, abs, exit

// ---------- constants ----------
#define MAXO 10          // maximum number of orders
#define MAXB 10          // maximum number of batches
#define DEPOT_R (-1)     // the depot (packing point) is at the entrance, just before aisle 0
#define DEPOT_C 0        // shelf 0
#define WALK_SPEED 10    // a picker walks 10 steps in one minute

// ---------- warehouse data (global, so every function can use it) ----------

// names of the products (row = aisle, column = shelf)
char names[3][3][15] =
{
    {"Water", "Juice", "Cola"},         // row 0: drinks
    {"Chips", "Biscuits", "Namkeen"},   // row 1: snacks
    {"Soap", "Detergent", "Shampoo"}    // row 2: household
};

// how many units of each product are in stock
int amount[3][3] =
{
    {50, 40, 30},   // Water, Juice, Cola
    {80, 60, 70},   // Chips, Biscuits, Namkeen
    {20, 25, 15}    // Soap, Detergent, Shampoo
};

// weight of ONE unit of each product
int weight[3][3] =
{
    {2, 2, 3},      // Water, Juice, Cola
    {1, 1, 1},      // Chips, Biscuits, Namkeen
    {1, 4, 2}       // Soap, Detergent, Shampoo
};

// volume of ONE unit of each product
int volume[3][3] =
{
    {2, 1, 3},      // Water, Juice, Cola
    {2, 1, 1},      // Chips, Biscuits, Namkeen
    {1, 4, 2}       // Soap, Detergent, Shampoo
};

// ---------- the 3 pickers, from the smallest cart to the biggest cart ----------
// index 0 = Picker 1, index 1 = Picker 2, index 2 = Picker 3
int pickMaxW[3] = {15, 40, 80};     // most weight each picker can carry
int pickMaxV[3] = {20, 60, 120};    // most volume each picker can carry
int pickMaxI[3] = {10, 30, 60};     // most items (units) each picker can carry

// ---------- orders ----------
int numOrders;           // how many orders the manager entered
int *ordId[MAXO];        // ordId[o] points to a malloc'd array of product ids of order o
int *ordQty[MAXO];       // ordQty[o] points to a malloc'd array of quantities of order o
int ordCount[MAXO];      // how many different products are in order o
int ordW[MAXO];          // total weight of order o
int ordV[MAXO];          // total volume of order o
int ordI[MAXO];          // total number of items (units) of order o
int ordOk[MAXO];         // 1 = some picker can carry this order, 0 = too big

// ---------- batches (a batch = a group of orders picked in one trip) ----------
int numBatches;              // how many batches were made
int batchPicker[MAXB];       // which picker (0, 1 or 2) does this batch
int batchW[MAXB];            // total weight of the batch
int batchV[MAXB];            // total volume of the batch
int batchI[MAXB];            // total items of the batch
int batchNum[MAXB];          // how many orders are in the batch
int batchOrd[MAXB][MAXO];    // which orders are in the batch

// ---------- small helper functions ----------

// Manhattan distance between two points: |r1-r2| + |c1-c2|
// (a picker can only walk along aisles, not diagonally)
int manhattan(int r1, int c1, int r2, int c2)
{
    return abs(r1 - r2) + abs(c1 - c2);
}

// Read a whole number between low and high from the keyboard.
// It keeps asking until the manager types a correct number.
int readInRange(int low, int high)
{
    int x = 0;
    int res;
    int ch;
    int ok = 0;

    while (ok == 0)
    {
        res = scanf("%d", &x);

        if (res == EOF)
        {
            // no more input at all, stop the program
            printf("\nInput ended. Program stopped.\n");
            exit(0);
        }

        if (res != 1)
        {
            // the manager typed letters instead of a number
            printf("Enter numbers only. Try again: ");
            while ((ch = getchar()) != '\n' && ch != EOF);   // throw away the wrong input
        }
        else if (x < low || x > high)
        {
            printf("Enter a number from %d to %d: ", low, high);
        }
        else
        {
            ok = 1;   // the number is correct
        }
    }
    return x;
}

// Check if picker p can carry this weight, volume and number of items
// returns 1 if yes, 0 if no
int pickerFits(int p, int w, int v, int items)
{
    if (w <= pickMaxW[p] && v <= pickMaxV[p] && items <= pickMaxI[p])
    {
        return 1;
    }
    return 0;
}

// Find the smallest picker who can carry this load alone
// returns the picker index (0, 1 or 2), or -1 if nobody can carry it
int bestPicker(int w, int v, int items)
{
    int p;
    for (p = 0; p < 3; p++)
    {
        if (pickerFits(p, w, v, items) == 1)
        {
            return p;
        }
    }
    return -1;
}

// ---------- taking the orders ----------

// Ask the manager for all the orders.
// Every order has a different number of products, so its arrays are made with malloc.
void inputOrders()
{
    int o, i, j, m, id, r, c, qty, repeated;

    printf("\nEnter number of orders (1 to %d): ", MAXO);
    numOrders = readInRange(1, MAXO);

    for (o = 0; o < numOrders; o++)
    {
        printf("\n=== ORDER %d ===\n", o + 1);
        printf("How many different products are in order %d (1 to 9): ", o + 1);
        m = readInRange(1, 9);

        // dynamic memory: make space for exactly m products
        ordCount[o] = m;
        ordId[o] = (int *) malloc(m * sizeof(int));
        ordQty[o] = (int *) malloc(m * sizeof(int));

        ordW[o] = 0;
        ordV[o] = 0;
        ordI[o] = 0;

        i = 0;
        while (i < m)
        {
            printf("Product %d: enter its id (row digit then column digit, e.g. 12 for _12): ", i + 1);
            id = readInRange(0, 22);
            r = id / 10;   // first digit is the row (aisle)
            c = id % 10;   // last digit is the column (shelf)

            // the column must be 0, 1 or 2
            if (c > 2)
            {
                printf("No product with this id. Try again.\n");
                continue;
            }

            // the same product cannot be entered twice in one order
            repeated = 0;
            for (j = 0; j < i; j++)
            {
                if (ordId[o][j] == id)
                {
                    repeated = 1;
                }
            }
            if (repeated == 1)
            {
                printf("This product is already in this order. Enter a different id.\n");
                continue;
            }

            // check the stock
            if (amount[r][c] == 0)
            {
                printf("%s is out of stock. Enter a different id.\n", names[r][c]);
                continue;
            }

            printf("Selected _%d%d (%s), stock left: %d. Enter quantity: ", r, c, names[r][c], amount[r][c]);
            qty = readInRange(1, amount[r][c]);

            // save the product in the order
            ordId[o][i] = id;
            ordQty[o][i] = qty;

            // reduce the stock
            amount[r][c] = amount[r][c] - qty;

            // add to the totals of this order (weight and volume are for ONE unit)
            ordW[o] = ordW[o] + weight[r][c] * qty;
            ordV[o] = ordV[o] + volume[r][c] * qty;
            ordI[o] = ordI[o] + qty;

            i++;   // one more product taken
        }
    }
}

// Print every order with its totals and check if any picker can carry it
void showOrders()
{
    int o, i, r, c;

    printf("\n================ ORDER SUMMARY ================\n");
    for (o = 0; o < numOrders; o++)
    {
        printf("Order %d: ", o + 1);
        for (i = 0; i < ordCount[o]; i++)
        {
            r = ordId[o][i] / 10;
            c = ordId[o][i] % 10;
            printf("%d x %s", ordQty[o][i], names[r][c]);
            if (i < ordCount[o] - 1)
            {
                printf(", ");
            }
        }
        printf("\n   items %d, weight %d, volume %d", ordI[o], ordW[o], ordV[o]);

        if (bestPicker(ordW[o], ordV[o], ordI[o]) == -1)
        {
            ordOk[o] = 0;
            printf("  -> TOO BIG for every picker, skipped");
        }
        else
        {
            ordOk[o] = 1;
        }
        printf("\n");
    }
}

// ---------- routing ----------

// Add the products of order o into the need table
// need[r][c] = how many units must be picked from aisle r, shelf c
void addOrderToNeed(int need[3][3], int o)
{
    int i, r, c;
    for (i = 0; i < ordCount[o]; i++)
    {
        r = ordId[o][i] / 10;
        c = ordId[o][i] % 10;
        need[r][c] = need[r][c] + ordQty[o][i];
    }
}

// Nearest-neighbor routing:
// start at the depot, always walk to the closest shelf that still has to be visited,
// and at the end walk back to the depot.
// If show is 1, the pick list is printed. The total walking distance is returned.
int route(int need[3][3], int show)
{
    int visited[3][3];
    int r, c;
    int left = 0;                  // how many shelves still to visit
    int curR = DEPOT_R;            // current position of the picker
    int curC = DEPOT_C;
    int total = 0;                 // total distance walked
    int step = 0;
    int bestR = 0, bestC = 0, bestD, d;

    // count the shelves to visit and clear the visited table
    for (r = 0; r < 3; r++)
    {
        for (c = 0; c < 3; c++)
        {
            visited[r][c] = 0;
            if (need[r][c] > 0)
            {
                left++;
            }
        }
    }

    while (left > 0)
    {
        bestD = 1000;   // a very big number to start with

        // look for the closest shelf that is not visited yet
        for (r = 0; r < 3; r++)
        {
            for (c = 0; c < 3; c++)
            {
                if (need[r][c] > 0 && visited[r][c] == 0)
                {
                    d = manhattan(curR, curC, r, c);
                    if (d < bestD)
                    {
                        bestD = d;
                        bestR = r;
                        bestC = c;
                    }
                }
            }
        }

        // walk to that shelf
        visited[bestR][bestC] = 1;
        total = total + bestD;
        step++;
        left--;

        if (show == 1)
        {
            printf("    %d. Go to _%d%d (Aisle %d, Shelf %d): pick %d x %s   (walk %d, total %d)\n",
                   step, bestR, bestC, bestR, bestC, need[bestR][bestC], names[bestR][bestC], bestD, total);
        }

        curR = bestR;
        curC = bestC;
    }

    // walk back to the depot
    d = manhattan(curR, curC, DEPOT_R, DEPOT_C);
    total = total + d;
    if (show == 1)
    {
        printf("    Return to depot                                   (walk %d, total %d)\n", d, total);
    }

    return total;
}

// ---------- batching ----------

// Group the orders into batches (trips).
// Steps: sort the orders heaviest first, then put each order in the first batch
// that can still carry it. If the batch needs a bigger picker, move up to a bigger picker.
// If it fits in no batch, start a new batch with the smallest picker that can carry it.
void createBatches()
{
    int sorted[MAXO];
    int i, j, k, o, b, p, temp, placed;

    // sorting: list the order numbers, then bubble sort them by weight (heaviest first)
    for (i = 0; i < numOrders; i++)
    {
        sorted[i] = i;
    }
    for (i = 0; i < numOrders - 1; i++)
    {
        for (j = 0; j < numOrders - 1 - i; j++)
        {
            if (ordW[sorted[j]] < ordW[sorted[j + 1]])
            {
                temp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = temp;
            }
        }
    }

    numBatches = 0;

    for (k = 0; k < numOrders; k++)
    {
        o = sorted[k];   // the order we are placing now

        if (ordOk[o] == 0)
        {
            continue;    // this order is too big for every picker
        }

        // try to put it in a batch that already exists
        placed = 0;
        for (b = 0; b < numBatches && placed == 0; b++)
        {
            for (p = batchPicker[b]; p < 3 && placed == 0; p++)
            {
                if (pickerFits(p, batchW[b] + ordW[o], batchV[b] + ordV[o], batchI[b] + ordI[o]) == 1)
                {
                    batchPicker[b] = p;
                    batchW[b] = batchW[b] + ordW[o];
                    batchV[b] = batchV[b] + ordV[o];
                    batchI[b] = batchI[b] + ordI[o];
                    batchOrd[b][batchNum[b]] = o;
                    batchNum[b]++;
                    placed = 1;
                }
            }
        }

        // if it did not fit anywhere, start a new batch
        if (placed == 0)
        {
            b = numBatches;
            batchPicker[b] = bestPicker(ordW[o], ordV[o], ordI[o]);
            batchW[b] = ordW[o];
            batchV[b] = ordV[o];
            batchI[b] = ordI[o];
            batchNum[b] = 1;
            batchOrd[b][0] = o;
            numBatches++;
        }
    }
}

// ---------- the two scenarios ----------

// Scenario 1: every order is picked in its own trip. Returns the total distance.
int singlePicking()
{
    int o, r, c, dist;
    int total = 0;
    int need[3][3];

    printf("\n========== SCENARIO 1: SINGLE-ORDER PICKING ==========\n");

    for (o = 0; o < numOrders; o++)
    {
        if (ordOk[o] == 0)
        {
            continue;   // skip orders that are too big
        }

        // clear the need table, then add only this order
        for (r = 0; r < 3; r++)
        {
            for (c = 0; c < 3; c++)
            {
                need[r][c] = 0;
            }
        }
        addOrderToNeed(need, o);

        printf("Trip for Order %d (Picker %d):\n", o + 1, bestPicker(ordW[o], ordV[o], ordI[o]) + 1);
        dist = route(need, 1);
        printf("  Trip distance: %d\n", dist);
        total = total + dist;
    }

    printf("Total distance (single picking): %d\n", total);
    return total;
}

// Scenario 2: orders are grouped in batches, one trip for each batch. Returns the total distance.
int batchedPicking()
{
    int b, k, r, c, dist;
    int total = 0;
    int need[3][3];

    createBatches();

    printf("\n============ SCENARIO 2: BATCHED PICKING ============\n");

    for (b = 0; b < numBatches; b++)
    {
        // batch manifest: which orders are inside, who carries it, and the load
        printf("Batch %d manifest: orders [", b + 1);
        for (k = 0; k < batchNum[b]; k++)
        {
            printf(" %d", batchOrd[b][k] + 1);
        }
        printf(" ]  Picker %d\n", batchPicker[b] + 1);
        printf("  Weight %d/%d, Volume %d/%d, Items %d/%d\n",
               batchW[b], pickMaxW[batchPicker[b]],
               batchV[b], pickMaxV[batchPicker[b]],
               batchI[b], pickMaxI[batchPicker[b]]);

        // clear the need table, then add every order of this batch
        for (r = 0; r < 3; r++)
        {
            for (c = 0; c < 3; c++)
            {
                need[r][c] = 0;
            }
        }
        for (k = 0; k < batchNum[b]; k++)
        {
            addOrderToNeed(need, batchOrd[b][k]);
        }

        printf("  Pick list:\n");
        dist = route(need, 1);
        printf("  Batch distance: %d\n", dist);
        total = total + dist;
    }

    printf("Total distance (batched picking): %d\n", total);
    return total;
}

// ---------- main ----------

int main()
{
    // one pointer for each product, named _(row)(column)
    // each pointer stores the address of its product's name
    char *_00 = names[0][0];   // points to "Water"
    char *_01 = names[0][1];   // points to "Juice"
    char *_02 = names[0][2];   // points to "Cola"

    char *_10 = names[1][0];   // points to "Chips"
    char *_11 = names[1][1];   // points to "Biscuits"
    char *_12 = names[1][2];   // points to "Namkeen"

    char *_20 = names[2][0];   // points to "Soap"
    char *_21 = names[2][1];   // points to "Detergent"
    char *_22 = names[2][2];   // points to "Shampoo"

    int r, c, p, o;
    int singleCost, batchCost, saved;

    // ----- show the product list -----
    printf("================ PRODUCT LIST ================\n");
    for (r = 0; r < 3; r++)
    {
        for (c = 0; c < 3; c++)
        {
            // _%d%d prints "_" then the row then the column, so r=0, c=1 gives _01
            printf("_%d%d: %-10s Aisle %d, Shelf %d | stock %d, weight %d, volume %d\n",
                   r, c, names[r][c], r, c, amount[r][c], weight[r][c], volume[r][c]);
        }
    }

    // ----- show the pickers -----
    printf("\n================== PICKERS ==================\n");
    for (p = 0; p < 3; p++)
    {
        printf("Picker %d: max weight %d, max volume %d, max items %d\n",
               p + 1, pickMaxW[p], pickMaxV[p], pickMaxI[p]);
    }

    // ----- take the orders and show them -----
    inputOrders();
    showOrders();

    // ----- run both scenarios -----
    singleCost = singlePicking();
    batchCost = batchedPicking();

    // ----- compare the two scenarios -----
    printf("\n================== COMPARISON ==================\n");
    printf("Single-order picking: distance %d, time %.1f min\n", singleCost, (float) singleCost / WALK_SPEED);
    printf("Batched picking     : distance %d, time %.1f min\n", batchCost, (float) batchCost / WALK_SPEED);
    if (singleCost > 0)
    {
        saved = singleCost - batchCost;
        printf("Distance saved by batching: %d (%.1f%%)\n", saved, 100.0 * saved / singleCost);
    }
    else
    {
        printf("No order could be picked, so there is nothing to compare.\n");
    }

    // ----- free the dynamic memory -----
    for (o = 0; o < numOrders; o++)
    {
        free(ordId[o]);
        free(ordQty[o]);
    }

    // ----- show the pointers working -----
    // %s prints the name stored at the address in the pointer
    printf("\n================ POINTER CHECK ================\n");
    printf("_00 -> %s, _01 -> %s, _02 -> %s\n", _00, _01, _02);
    printf("_10 -> %s, _11 -> %s, _12 -> %s\n", _10, _11, _12);
    printf("_20 -> %s, _21 -> %s, _22 -> %s\n", _20, _21, _22);

    return 0;
}
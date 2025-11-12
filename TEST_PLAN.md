# System Testing Plan - Nov 11, 2025
**Deadline: Nov 14 (Friday) - 3 Days Remaining**

## Pre-Test Setup
```bash
# Clean slate
cd "/Users/wesleymarizane/UNI/YEAR 4/FALL 25/COMP 7212 (Operating:Distrib Sys)/project"
rm -f broker_log.txt transactions.txt
```

---

## TEST 1: Basic Pipeline (All 100 Transactions)
**Goal:** Verify broker correctly routes all messages from producer to consumer

### Steps:
1. **Terminal 1 - Start Broker:**
   ```bash
   ./broker_exe 9100 9200
   ```
   Expected: "Loaded 0 unacked messages from log"

2. **Terminal 2 - Start Consumer:**
   ```bash
   ./consumer_exe --connect 127.0.0.1 9200
   ```
   Expected: "Connected to broker at 127.0.0.1:9200"

3. **Terminal 3 - Run Producer:**
   ```bash
   ./producer_exe 127.0.0.1 9100
   ```
   Expected: "Finished streaming 100 transactions to socket."

### Success Criteria:
- ✅ Consumer shows: "Received 100 transactions"
- ✅ Consumer shows: "Valid transactions: 100"
- ✅ Consumer shows total amount and average
- ✅ No errors in broker output
- ✅ `broker_log.txt` exists with 100 lines (check: `wc -l broker_log.txt`)

**Action:** Copy all 3 terminal outputs to `test1_output.txt`

---

## TEST 2: Consumer Failure & Redelivery (CRITICAL FOR DEMO)
**Goal:** Prove fault tolerance - messages are not lost when consumer crashes

### Steps:
1. **Clean and restart broker (Terminal 1):**
   ```bash
   rm broker_log.txt
   ./broker_exe 9100 9200
   ```

2. **Start consumer (Terminal 2):**
   ```bash
   ./consumer_exe --connect 127.0.0.1 9200
   ```

3. **Start producer (Terminal 3):**
   ```bash
   ./producer_exe 127.0.0.1 9100
   ```

4. **AFTER ~30 TRANSACTIONS, kill consumer (Terminal 2):**
   Press `Ctrl+C`
   
   Watch Terminal 1 (broker) - should see messages about consumer disconnect

5. **Restart consumer immediately (Terminal 2):**
   ```bash
   ./consumer_exe --connect 127.0.0.1 9200
   ```

### Success Criteria:
- ✅ Broker shows: "Consumer disconnected (requeued message X)"
- ✅ Consumer receives messages after restart
- ✅ Final count: Consumer shows 100 total transactions processed
- ✅ No duplicate messages (verify transaction IDs are unique)

**Action:** Copy all 3 terminal outputs to `test2_output.txt`

---

## TEST 3: Broker Restart & Log Replay
**Goal:** Prove persistence - messages survive broker crash

### Steps:
1. **Start broker (Terminal 1):**
   ```bash
   rm broker_log.txt
   ./broker_exe 9100 9200
   ```

2. **Start consumer (Terminal 2):**
   ```bash
   ./consumer_exe --connect 127.0.0.1 9200
   ```

3. **Start producer (Terminal 3):**
   ```bash
   ./producer_exe 127.0.0.1 9100
   ```

4. **AFTER ~10 TRANSACTIONS, kill broker (Terminal 1):**
   Press `Ctrl+C`
   
   Check log: `wc -l broker_log.txt` (should show ~10 lines)

5. **Restart broker (Terminal 1):**
   ```bash
   ./broker_exe 9100 9200
   ```
   Expected: "Loaded X unacked messages from log" (where X > 0)

6. **Observe:** Consumer should receive those replayed messages

### Success Criteria:
- ✅ Broker loads unacked messages on restart
- ✅ Consumer receives replayed messages
- ✅ `broker_log.txt` shows correct message format

**Action:** Copy all 3 terminal outputs to `test3_output.txt`

---

## Quick Validation Commands

Check broker log format:
```bash
head -5 broker_log.txt
```
Expected format: `<message_id>|0|<transaction_data>`

Count messages in log:
```bash
wc -l broker_log.txt
```

Check for duplicates in consumer output:
```bash
# If you save consumer output with transaction IDs
grep "ID:" consumer_output.txt | sort | uniq -d
```

---

## After All Tests Pass

1. **Commit everything:**
   ```bash
   git add -A
   git commit -m "[Phase 4] Fault tolerance complete - tested and verified
   
   - ACK tracking with message IDs
   - Persistent broker_log.txt
   - Consumer failure redelivery (Test 2)
   - Broker restart recovery (Test 3)
   - All 100 transactions flow correctly (Test 1)"
   ```

2. **Push to remote:**
   ```bash
   git push origin main
   ```

---

## What to Report if Tests FAIL

For each failed test, provide:
1. Which test failed (1, 2, or 3)
2. Complete terminal outputs from all 3 terminals
3. Contents of `broker_log.txt` (first 10 lines: `head -10 broker_log.txt`)
4. Any error messages

---

## Next Steps After Testing (Nov 12-14)
- [ ] Scale test (10K transactions)
- [ ] Docker containerization
- [ ] Record demo video (15 min)
- [ ] Final submission materials

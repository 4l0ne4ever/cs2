#!/bin/bash
# run_all_tests.sh - Test Runner for All Tests (Phase 10)

echo "=========================================="
echo "  CS2 SKIN TRADING - TEST SUITE RUNNER"
echo "=========================================="
echo ""

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

PASSED=0
FAILED=0
TOTAL=0

# Function to run a test
run_test() {
    local test_name=$1
    local test_file=$2
    
    echo -n "Running $test_name... "
    
    if make $test_file > /dev/null 2>&1; then
        if ./tests/$test_file > /tmp/test_output_$$.txt 2>&1; then
            if grep -q "passed\|PASSED" /tmp/test_output_$$.txt; then
                echo -e "${GREEN}✓ PASSED${NC}"
                ((PASSED++))
            else
                echo -e "${RED}✗ FAILED${NC}"
                cat /tmp/test_output_$$.txt | tail -5
                ((FAILED++))
            fi
        else
            echo -e "${RED}✗ FAILED${NC}"
            cat /tmp/test_output_$$.txt | tail -5
            ((FAILED++))
        fi
    else
        echo -e "${YELLOW}⚠ SKIPPED (build failed)${NC}"
        ((FAILED++))
    fi
    
    ((TOTAL++))
    echo ""
}

# Unit Tests
echo "=== UNIT TESTS ==="
echo ""

run_test "SQLite" "test_sqlite"
run_test "Authentication" "test_auth"
run_test "Market" "test_market"
run_test "Trading" "test_trading"
run_test "Unbox Engine" "test_unbox"
run_test "Report System" "test_report"
run_test "Request Handler" "test_request_handler"

# Integration Tests
echo "=== INTEGRATION TESTS ==="
echo ""

run_test "Server Integration" "test_server_integration"
run_test "System Integration" "test_system_integration"
run_test "Multi-User Scenarios" "test_multi_user"

# Load Tests
echo "=== LOAD TESTS ==="
echo ""

run_test "Server Stress" "test_server_stress"
run_test "Memory Leak Detection" "test_memory_leak"
run_test "Comprehensive Load" "test_load_comprehensive"

# Summary
echo "=========================================="
echo "  TEST SUMMARY"
echo "=========================================="
echo ""
echo "Total Tests: $TOTAL"
echo -e "${GREEN}Passed: $PASSED${NC}"
echo -e "${RED}Failed: $FAILED${NC}"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ ALL TESTS PASSED!${NC}"
    exit 0
else
    echo -e "${RED}✗ SOME TESTS FAILED${NC}"
    exit 1
fi


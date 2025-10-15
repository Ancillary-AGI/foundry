#!/bin/bash

# Foundry IDE Performance Test Script
# Runs comprehensive performance benchmarks and generates reports

set -e

# Configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PERFORMANCE_DIR="$PROJECT_ROOT/performance"
REPORTS_DIR="$PROJECT_ROOT/reports"
ITERATIONS=100
CONCURRENT_USERS=10

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Create directories
mkdir -p "$PERFORMANCE_DIR"
mkdir -p "$REPORTS_DIR"

# Logging
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Performance metrics collection
collect_metrics() {
    local test_name="$1"
    local start_time="$2"
    local end_time="$3"
    local memory_usage="$4"
    local cpu_usage="$5"

    local duration=$((end_time - start_time))
    local memory_mb=$((memory_usage / 1024 / 1024))

    echo "$test_name,$duration,$memory_mb,$cpu_usage" >> "$PERFORMANCE_DIR/results.csv"
}

# Memory usage monitoring
get_memory_usage() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        ps aux --no-headers -o pmem -C "$1" | awk '{sum+=$1} END {print sum}'
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        ps aux -o pmem -C "$1" | tail -n 1 | awk '{print $4}'
    else
        echo "0"
    fi
}

# CPU usage monitoring
get_cpu_usage() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        ps aux --no-headers -o pcpu -C "$1" | awk '{sum+=$1} END {print sum}'
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        ps aux -o pcpu -C "$1" | tail -n 1 | awk '{print $3}'
    else
        echo "0"
    fi
}

# Engine Core Performance Tests
test_engine_core_performance() {
    log_info "Testing Engine Core Performance..."

    cd "$PROJECT_ROOT/build"

    # Vector operations benchmark
    log_info "Running vector operations benchmark..."
    local start_time=$(date +%s)
    # Run vector benchmark test
    if ./tests/TestEngineCore --gtest_filter="*Vector*" --gtest_repeat=$ITERATIONS >/dev/null 2>&1; then
        local end_time=$(date +%s)
        local memory=$(get_memory_usage "TestEngineCore")
        local cpu=$(get_cpu_usage "TestEngineCore")
        collect_metrics "vector_operations" "$start_time" "$end_time" "$memory" "$cpu"
        log_success "Vector operations benchmark completed"
    else
        log_warning "Vector operations benchmark failed"
    fi

    # Matrix operations benchmark
    log_info "Running matrix operations benchmark..."
    start_time=$(date +%s)
    if ./tests/TestEngineCore --gtest_filter="*Matrix*" --gtest_repeat=$((ITERATIONS/10)) >/dev/null 2>&1; then
        end_time=$(date +%s)
        memory=$(get_memory_usage "TestEngineCore")
        cpu=$(get_cpu_usage "TestEngineCore")
        collect_metrics "matrix_operations" "$start_time" "$end_time" "$memory" "$cpu"
        log_success "Matrix operations benchmark completed"
    else
        log_warning "Matrix operations benchmark failed"
    fi

    # Entity creation benchmark
    log_info "Running entity creation benchmark..."
    start_time=$(date +%s)
    if ./tests/TestEngineCore --gtest_filter="*EntityCreation*" --gtest_repeat=$((ITERATIONS/20)) >/dev/null 2>&1; then
        end_time=$(date +%s)
        memory=$(get_memory_usage "TestEngineCore")
        cpu=$(get_cpu_usage "TestEngineCore")
        collect_metrics "entity_creation" "$start_time" "$end_time" "$memory" "$cpu"
        log_success "Entity creation benchmark completed"
    else
        log_warning "Entity creation benchmark failed"
    fi
}

# IDE Performance Tests
test_ide_performance() {
    log_info "Testing IDE Performance..."

    cd "$PROJECT_ROOT/ide"

    # Code editor performance
    log_info "Running code editor performance tests..."
    local start_time=$(date +%s)
    if ./gradlew performanceTest --no-daemon -Dtest.single=CodeEditorPerformance >/dev/null 2>&1; then
        local end_time=$(date +%s)
        collect_metrics "code_editor" "$start_time" "$end_time" "0" "0"
        log_success "Code editor performance test completed"
    else
        log_warning "Code editor performance test failed"
    fi

    # Plugin system performance
    log_info "Running plugin system performance tests..."
    start_time=$(date +%s)
    if ./gradlew performanceTest --no-daemon -Dtest.single=PluginPerformance >/dev/null 2>&1; then
        end_time=$(date +%s)
        collect_metrics "plugin_system" "$start_time" "$end_time" "0" "0"
        log_success "Plugin system performance test completed"
    else
        log_warning "Plugin system performance test failed"
    fi

    # UI responsiveness
    log_info "Running UI responsiveness tests..."
    start_time=$(date +%s)
    if ./gradlew performanceTest --no-daemon -Dtest.single=UIResponsiveness >/dev/null 2>&1; then
        end_time=$(date +%s)
        collect_metrics "ui_responsiveness" "$start_time" "$end_time" "0" "0"
        log_success "UI responsiveness test completed"
    else
        log_warning "UI responsiveness test failed"
    fi
}

# MCP and Agent Performance Tests
test_mcp_agent_performance() {
    log_info "Testing MCP and Agent System Performance..."

    cd "$PROJECT_ROOT/ide"

    # Agent execution performance
    log_info "Running agent execution performance tests..."
    local start_time=$(date +%s)
    if ./gradlew performanceTest --no-daemon -Dtest.single=AgentExecutionPerformance >/dev/null 2>&1; then
        local end_time=$(date +%s)
        collect_metrics "agent_execution" "$start_time" "$end_time" "0" "0"
        log_success "Agent execution performance test completed"
    else
        log_warning "Agent execution performance test failed"
    fi

    # Collaborative task performance
    log_info "Running collaborative task performance tests..."
    start_time=$(date +%s)
    if ./gradlew performanceTest --no-daemon -Dtest.single=CollaborativeTaskPerformance >/dev/null 2>&1; then
        end_time=$(date +%s)
        collect_metrics "collaborative_tasks" "$start_time" "$end_time" "0" "0"
        log_success "Collaborative task performance test completed"
    else
        log_warning "Collaborative task performance test failed"
    fi

    # Memory system performance
    log_info "Running memory system performance tests..."
    start_time=$(date +%s)
    if ./gradlew performanceTest --no-daemon -Dtest.single=MemorySystemPerformance >/dev/null 2>&1; then
        end_time=$(date +%s)
        collect_metrics "memory_system" "$start_time" "$end_time" "0" "0"
        log_success "Memory system performance test completed"
    else
        log_warning "Memory system performance test failed"
    fi
}

# Load Testing
run_load_tests() {
    log_info "Running Load Tests..."

    cd "$PROJECT_ROOT/ide"

    # Concurrent user simulation
    log_info "Running concurrent user load test..."
    local start_time=$(date +%s)
    if ./gradlew loadTest --no-daemon -Dload.users=$CONCURRENT_USERS -Dload.duration=60 >/dev/null 2>&1; then
        local end_time=$(date +%s)
        collect_metrics "concurrent_users_$CONCURRENT_USERS" "$start_time" "$end_time" "0" "0"
        log_success "Concurrent user load test completed"
    else
        log_warning "Concurrent user load test failed"
    fi

    # Memory leak detection
    log_info "Running memory leak detection..."
    start_time=$(date +%s)
    if ./gradlew memoryTest --no-daemon >/dev/null 2>&1; then
        end_time=$(date +%s)
        collect_metrics "memory_leak_detection" "$start_time" "$end_time" "0" "0"
        log_success "Memory leak detection completed"
    else
        log_warning "Memory leak detection failed"
    fi

    # Stress testing
    log_info "Running stress tests..."
    start_time=$(date +%s)
    if ./gradlew stressTest --no-daemon >/dev/null 2>&1; then
        end_time=$(date +%s)
        collect_metrics "stress_test" "$start_time" "$end_time" "0" "0"
        log_success "Stress test completed"
    else
        log_warning "Stress test failed"
    fi
}

# Memory Usage Analysis
analyze_memory_usage() {
    log_info "Analyzing Memory Usage..."

    # Get baseline memory usage
    local baseline_memory=$(ps aux --no-headers -o pmem -C "java" | awk '{sum+=$1} END {print sum}')

    # Run memory-intensive operations
    cd "$PROJECT_ROOT/ide"
    ./gradlew memoryIntensiveTest --no-daemon >/dev/null 2>&1

    # Get peak memory usage
    local peak_memory=$(ps aux --no-headers -o pmem -C "java" | awk '{sum+=$1} END {print sum}')

    # Calculate memory delta
    local memory_delta=$((peak_memory - baseline_memory))

    echo "memory_analysis,$baseline_memory,$peak_memory,$memory_delta" >> "$PERFORMANCE_DIR/memory_analysis.csv"

    log_success "Memory analysis completed. Baseline: ${baseline_memory}MB, Peak: ${peak_memory}MB, Delta: ${memory_delta}MB"
}

# Generate Performance Report
generate_performance_report() {
    log_info "Generating Performance Report..."

    # Create CSV header if it doesn't exist
    if [ ! -f "$PERFORMANCE_DIR/results.csv" ]; then
        echo "test_name,duration_seconds,memory_mb,cpu_percent" > "$PERFORMANCE_DIR/results.csv"
    fi

    # Create memory analysis header if it doesn't exist
    if [ ! -f "$PERFORMANCE_DIR/memory_analysis.csv" ]; then
        echo "analysis_type,baseline_mb,peak_mb,delta_mb" > "$PERFORMANCE_DIR/memory_analysis.csv"
    fi

    # Generate HTML report
    cat > "$REPORTS_DIR/performance-report.html" << EOF
<!DOCTYPE html>
<html>
<head>
    <title>Foundry IDE Performance Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .summary { background: #f0f0f0; padding: 20px; border-radius: 5px; margin-bottom: 20px; }
        .metrics { margin: 20px 0; }
        .chart { width: 100%; height: 400px; border: 1px solid #ddd; margin: 20px 0; }
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
        .passed { color: green; }
        .failed { color: red; }
        .warning { color: orange; }
    </style>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
</head>
<body>
    <h1>Foundry IDE Performance Report</h1>
    <div class="summary">
        <h2>Performance Summary</h2>
        <p><strong>Generated:</strong> $(date)</p>
        <p><strong>Platform:</strong> $(uname -a)</p>
        <p><strong>Test Iterations:</strong> $ITERATIONS</p>
        <p><strong>Concurrent Users:</strong> $CONCURRENT_USERS</p>
    </div>

    <div class="metrics">
        <h2>Performance Metrics</h2>
        <table>
            <tr><th>Test</th><th>Duration (s)</th><th>Memory (MB)</th><th>CPU (%)</th><th>Status</th></tr>
EOF

    # Add performance data to HTML
    if [ -f "$PERFORMANCE_DIR/results.csv" ]; then
        tail -n +2 "$PERFORMANCE_DIR/results.csv" | while IFS=',' read -r test_name duration memory cpu; do
            local status="passed"
            if [ "$duration" -gt 300 ]; then status="warning"; fi  # More than 5 minutes
            if [ "$memory" -gt 1024 ]; then status="warning"; fi  # More than 1GB

            echo "            <tr><td>$test_name</td><td>$duration</td><td>$memory</td><td>$cpu</td><td class=\"$status\">${status^}</td></tr>" >> "$REPORTS_DIR/performance-report.html"
        done
    fi

    cat >> "$REPORTS_DIR/performance-report.html" << EOF
        </table>
    </div>

    <div class="memory-analysis">
        <h2>Memory Analysis</h2>
        <table>
            <tr><th>Analysis Type</th><th>Baseline (MB)</th><th>Peak (MB)</th><th>Delta (MB)</th></tr>
EOF

    # Add memory analysis data
    if [ -f "$PERFORMANCE_DIR/memory_analysis.csv" ]; then
        tail -n +2 "$PERFORMANCE_DIR/memory_analysis.csv" | while IFS=',' read -r analysis_type baseline peak delta; do
            echo "            <tr><td>$analysis_type</td><td>$baseline</td><td>$peak</td><td>$delta</td></tr>" >> "$REPORTS_DIR/performance-report.html"
        done
    fi

    cat >> "$REPORTS_DIR/performance-report.html" << EOF
        </table>
    </div>

    <div class="recommendations">
        <h2>Performance Recommendations</h2>
        <ul>
            <li>Monitor memory usage during extended IDE sessions</li>
            <li>Consider optimizing vector/matrix operations for better performance</li>
            <li>Implement lazy loading for large projects</li>
            <li>Profile agent execution times for optimization opportunities</li>
            <li>Monitor CPU usage during peak loads</li>
        </ul>
    </div>

    <div class="chart">
        <canvas id="performanceChart"></canvas>
    </div>

    <script>
        // Load performance data and create chart
        fetch('$PERFORMANCE_DIR/results.csv')
            .then(response => response.text())
            .then(data => {
                const lines = data.trim().split('\n').slice(1); // Skip header
                const labels = [];
                const durations = [];
                const memory = [];
                const cpu = [];

                lines.forEach(line => {
                    const [testName, duration, mem, cpuVal] = line.split(',');
                    labels.push(testName);
                    durations.push(parseFloat(duration));
                    memory.push(parseFloat(mem));
                    cpu.push(parseFloat(cpuVal));
                });

                const ctx = document.getElementById('performanceChart').getContext('2d');
                new Chart(ctx, {
                    type: 'bar',
                    data: {
                        labels: labels,
                        datasets: [{
                            label: 'Duration (seconds)',
                            data: durations,
                            backgroundColor: 'rgba(75, 192, 192, 0.2)',
                            borderColor: 'rgba(75, 192, 192, 1)',
                            borderWidth: 1
                        }, {
                            label: 'Memory (MB)',
                            data: memory,
                            backgroundColor: 'rgba(153, 102, 255, 0.2)',
                            borderColor: 'rgba(153, 102, 255, 1)',
                            borderWidth: 1
                        }]
                    },
                    options: {
                        scales: {
                            y: {
                                beginAtZero: true
                            }
                        }
                    }
                });
            })
            .catch(error => console.error('Error loading performance data:', error));
    </script>
</body>
</html>
EOF

    log_success "Performance report generated: $REPORTS_DIR/performance-report.html"
}

# Main execution
main() {
    local start_time=$(date +%s)

    log_info "Starting Foundry IDE Performance Testing"
    log_info "Performance Results: $PERFORMANCE_DIR"
    log_info "Reports: $REPORTS_DIR"

    # Initialize results file
    echo "test_name,duration_seconds,memory_mb,cpu_percent" > "$PERFORMANCE_DIR/results.csv"
    echo "analysis_type,baseline_mb,peak_mb,delta_mb" > "$PERFORMANCE_DIR/memory_analysis.csv"

    # Run performance tests
    test_engine_core_performance
    test_ide_performance
    test_mcp_agent_performance
    run_load_tests
    analyze_memory_usage

    # Generate report
    generate_performance_report

    local end_time=$(date +%s)
    local duration=$((end_time - start_time))

    log_success "Performance testing completed in ${duration}s"
    log_info "Results saved to: $PERFORMANCE_DIR"
    log_info "Reports saved to: $REPORTS_DIR"
}

# Handle command line arguments
case "${1:-all}" in
    "engine")
        test_engine_core_performance
        ;;
    "ide")
        test_ide_performance
        ;;
    "mcp")
        test_mcp_agent_performance
        ;;
    "load")
        run_load_tests
        ;;
    "memory")
        analyze_memory_usage
        ;;
    "report")
        generate_performance_report
        ;;
    "all"|*)
        main
        ;;
esac
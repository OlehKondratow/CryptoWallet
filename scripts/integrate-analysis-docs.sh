#!/bin/bash

# CryptoWallet Documentation Auto-Generation Integration Script
# Интеграция документации в автоматическую систему подготовки
# Integracja dokumentacji w system automatycznego generowania

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
DOCS_DIR="$PROJECT_ROOT/docs_src"
ANALYSIS_DIR="$DOCS_DIR/analysis"
BUILD_DIR="$PROJECT_ROOT/build/docs"
OUTPUT_DIR="${BUILD_DIR}/analysis"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}CryptoWallet Documentation Integration${NC}"
echo -e "${BLUE}========================================${NC}"

# Check if analysis directory exists
if [ ! -d "$ANALYSIS_DIR" ]; then
    echo -e "${YELLOW}Creating analysis directory...${NC}"
    mkdir -p "$ANALYSIS_DIR"
fi

echo -e "${GREEN}✓ Analysis directory: $ANALYSIS_DIR${NC}"

# Function to build HTML from markdown
build_html() {
    local md_file="$1"
    local html_file="$2"
    
    if command -v pandoc &> /dev/null; then
        echo "  Converting: $(basename $md_file) → $(basename $html_file)"
        pandoc -f markdown -t html5 \
            --standalone \
            --css /style.css \
            -o "$html_file" "$md_file"
    else
        echo "  (pandoc not found, skipping HTML generation for: $(basename $md_file))"
    fi
}

# Function to generate documentation index
generate_index() {
    local output_file="$1"
    
    echo -e "${YELLOW}Generating documentation index...${NC}"
    
    # This would be integrated with your mkdocs/sphinx system
    # For now, we just copy the INDEX.md to output
    
    if [ -f "$ANALYSIS_DIR/INDEX.md" ]; then
        mkdir -p "$OUTPUT_DIR"
        cp "$ANALYSIS_DIR/INDEX.md" "$OUTPUT_DIR/index.md"
        echo -e "${GREEN}✓ Documentation index created${NC}"
    fi
}

# Function to validate markdown files
validate_markdown() {
    echo -e "${YELLOW}Validating markdown files...${NC}"
    
    local count=0
    for md_file in "$ANALYSIS_DIR"/*.md; do
        if [ -f "$md_file" ]; then
            # Check for basic markdown structure
            if grep -q "^#" "$md_file"; then
                ((count++))
            else
                echo -e "${RED}⚠ Warning: No headers found in $(basename $md_file)${NC}"
            fi
        fi
    done
    
    echo -e "${GREEN}✓ Validated $count markdown files${NC}"
}

# Function to copy analysis files
copy_analysis_files() {
    echo -e "${YELLOW}Copying analysis documentation...${NC}"
    
    mkdir -p "$OUTPUT_DIR"
    cp -v "$ANALYSIS_DIR"/*.md "$OUTPUT_DIR/" 2>/dev/null || true
    
    echo -e "${GREEN}✓ Analysis files copied to build directory${NC}"
}

# Function to generate language statistics
generate_stats() {
    echo -e "${YELLOW}Generating documentation statistics...${NC}"
    
    local total_lines=0
    local total_files=0
    local en_files=0
    local ru_files=0
    local pl_files=0
    
    for md_file in "$ANALYSIS_DIR"/*.md; do
        if [ -f "$md_file" ]; then
            ((total_files++))
            lines=$(wc -l < "$md_file")
            total_lines=$((total_lines + lines))
            
            if [[ "$md_file" == *"_ru.md" ]]; then
                ((ru_files++))
            elif [[ "$md_file" == *"_pl.md" ]]; then
                ((pl_files++))
            else
                ((en_files++))
            fi
        fi
    done
    
    echo -e "${GREEN}Documentation Statistics:${NC}"
    echo "  Total Files: $total_files"
    echo "  Total Lines: $total_lines"
    echo "  English Files: $en_files"
    echo "  Russian Files: $ru_files"
    echo "  Polish Files: $pl_files"
}

# Function to integrate with mkdocs
integrate_mkdocs() {
    echo -e "${YELLOW}Integrating with documentation system...${NC}"
    
    if [ -f "$DOCS_DIR/mkdocs.yml" ]; then
        echo "  mkdocs.yml found, integrating..."
        # Add analysis section to mkdocs if not present
        if ! grep -q "Analysis" "$DOCS_DIR/mkdocs.yml"; then
            cat >> "$DOCS_DIR/mkdocs.yml" << 'EOF'

    - Analysis:
        - Overview: 'analysis/INDEX.md'
        - Comparison: 'analysis/PROJECTS_COMPARISON_AND_UPDATES.md'
        - Updates: 'analysis/UPDATES_SUMMARY.md'
        - Reference: 'analysis/QUICK_REFERENCE.md'
        - Architecture: 'analysis/ARCHITECTURE_DETAILED.md'
        - Dependencies: 'analysis/PROJECT_DEPENDENCIES.md'
        - Visuals: 'analysis/VISUAL_SUMMARY.md'
EOF
            echo -e "${GREEN}✓ mkdocs.yml updated${NC}"
        else
            echo "  Analysis section already present in mkdocs.yml"
        fi
    fi
}

# Main execution
main() {
    echo -e "${BLUE}Starting documentation integration...${NC}\n"
    
    # Step 1: Validate
    validate_markdown
    echo
    
    # Step 2: Generate stats
    generate_stats
    echo
    
    # Step 3: Copy files
    copy_analysis_files
    echo
    
    # Step 4: Generate index
    generate_index "$OUTPUT_DIR/index.md"
    echo
    
    # Step 5: Integrate with mkdocs (if available)
    integrate_mkdocs
    echo
    
    # Step 6: Optional HTML generation
    if [ "$1" == "--html" ] && command -v pandoc &> /dev/null; then
        echo -e "${YELLOW}Generating HTML files...${NC}"
        for md_file in "$ANALYSIS_DIR"/*.md; do
            if [ -f "$md_file" ]; then
                html_file="$OUTPUT_DIR/$(basename $md_file .md).html"
                build_html "$md_file" "$html_file"
            fi
        done
        echo -e "${GREEN}✓ HTML generation complete${NC}"
    fi
    
    echo
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}✓ Documentation integration complete!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo
    echo -e "${BLUE}Generated files located at:${NC}"
    echo "  $OUTPUT_DIR"
    echo
    echo -e "${BLUE}To view documentation:${NC}"
    echo "  1. English: docs_src/analysis/00_README_DOCUMENTATION.md"
    echo "  2. Russian: docs_src/analysis/00_README_DOCUMENTATION_ru.md"
    echo "  3. Polish:  docs_src/analysis/00_README_DOCUMENTATION_pl.md"
    echo
    echo -e "${BLUE}To serve with mkdocs:${NC}"
    echo "  make docs-serve"
    echo
}

# Run main function
main "$@"

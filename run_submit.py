import sys
try:
    from tools import submit
    submit(branch_name="palette-ui-scale", pr_title="🎨 Palette: Separate UI element and text scaling")
except Exception as e:
    print(f"Failed to submit: {e}")

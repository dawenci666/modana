mkdir -p frames_png
for f in frames/frame_*.dot; do
    base=$(basename "$f" .dot)
    neato -Tpng "$f" -o frames_png/${base}.png -Gstart=42 -Gmaxiter=500
done

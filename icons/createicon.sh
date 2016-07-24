for size in 16 22 24 32 48 64 128 256 512 1024 ; do
    inkscape -z -e app$size.png -w $size -h $size app.svg
    optipng app$size.png
done

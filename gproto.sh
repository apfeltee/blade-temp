
for item in "$@"; do
  cproto "$item" | perl -pe 's/\b_Bool\b/bool/g'
done


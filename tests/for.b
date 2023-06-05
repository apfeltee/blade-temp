foreach i in [1,2,3,4,5] {
  echo i
  if i == 4 break
}

var details = {name: 'Richard', age: 27, address: 'Nigeria'}

foreach x, y in details {
  echo '${x} = ${y}'

  foreach i, j in [6,7,8,9,10] {
    echo '${i} = ${j}'

    foreach i in [11,12,13,14,15] {
      echo i
    }
  }
}

foreach n in 'name' {
  echo n
}

foreach g in bytes([10, 21, 13, 47]) {
  echo g
}

class Iterable {
  var items = ['Richard', 'Alex', 'Justina']

  @iter(x) {
    return self.items[x]
  }

  @itern(x) {
    if x == nil return 0

    if x < self.items.length() - 1
      return x + 1
    return false
  }
}

foreach it in Iterable() {
  echo it
}

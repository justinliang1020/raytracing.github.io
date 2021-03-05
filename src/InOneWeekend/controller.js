function getAttribute(attribute) {
  var x = document.getElementById(attribute);
  switch(x.value) {
    case "low":
      return 1;
    case "medium":
      return 2;
    case "high":
      return 3;
  }
  return 4;
}


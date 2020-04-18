target triple = "le32-unknown-nacl"

define double @frem(double %numerator, double %denom) {
  %r = frem double %numerator, %denom
  ret double %r
}

define float @fremf(float %numerator, float %denom) {
  %r = frem float %numerator, %denom
  ret float %r
}

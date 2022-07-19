export function getRandomElement(a) {
  return a[Math.floor(Math.random() * a.length)];
}

export function shuffle(a) {
  for (let i = 0; i + 1 < a.length; ++i) {
    const j = i + Math.floor(Math.random() * (a.length - i));
    const tmp = a[i];
    a[i] = a[j];
    a[j] = tmp;
  }
}

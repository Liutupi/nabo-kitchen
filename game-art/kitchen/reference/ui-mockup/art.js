// 占位用的可爱扁平 SVG：菜品和食材。只用来定版式和气质，正式素材由 Codex 按 ART_BRIEF 画。
const OL = '#3B2730';
const svg = (w, h, vb, body) => `<svg width="${w}" height="${h}" viewBox="${vb}" xmlns="http://www.w3.org/2000/svg">${body}</svg>`;

function plate(cx, cy, rx, ry, oval) {
  return `<ellipse cx="${cx}" cy="${cy + 5}" rx="${rx + 2}" ry="${ry}" fill="#000" opacity=".22"/>
  <ellipse cx="${cx}" cy="${cy}" rx="${rx}" ry="${ry}" fill="#FFFFFF" stroke="${OL}" stroke-width="2.4"/>
  <ellipse cx="${cx}" cy="${cy}" rx="${rx * 0.74}" ry="${ry * 0.7}" fill="#EEF0F6"/>
  <path d="M${cx - rx * 0.86} ${cy - ry * 0.18} A ${rx} ${ry} 0 0 1 ${cx - rx * 0.2} ${cy - ry * 0.95}" stroke="#fff" stroke-width="3" fill="none" opacity=".9"/>`;
}
function eggChunk(x, y, s, r) {
  return `<g transform="translate(${x} ${y}) rotate(${r}) scale(${s})"><path d="M-11 -3 C-12 -11 -2 -13 4 -10 C12 -8 13 0 9 6 C5 11 -6 11 -10 5 Z" fill="#FFD447" stroke="${OL}" stroke-width="2"/><path d="M-6 -6 C-3 -9 2 -9 4 -7" stroke="#FFF3B0" stroke-width="2.4" fill="none" stroke-linecap="round"/><path d="M-4 4 C0 6 4 5 6 2" stroke="#F2A93B" stroke-width="2" fill="none" stroke-linecap="round"/></g>`;
}
function tomatoWedge(x, y, s, r) {
  return `<g transform="translate(${x} ${y}) rotate(${r}) scale(${s})"><path d="M-12 4 C-10 -8 6 -12 12 -4 C8 4 -2 9 -12 4 Z" fill="#FF5A4C" stroke="${OL}" stroke-width="2"/><path d="M-5 0 C-2 -3 3 -4 6 -3" stroke="#FFB0A4" stroke-width="2.2" fill="none" stroke-linecap="round"/><circle cx="1" cy="2" r="1.3" fill="#FFE07A"/></g>`;
}
function scallions(pts) {
  return pts.map(([x, y, r]) => `<rect x="${x}" y="${y}" width="5" height="3" rx="1.4" fill="#5BD08A" stroke="${OL}" stroke-width="1" transform="rotate(${r} ${x} ${y})"/>`).join('');
}
function shine(pts) {
  return pts.map(([x, y, s]) => `<path d="M${x} ${y - s} L${x + s * 0.3} ${y - s * 0.3} L${x + s} ${y} L${x + s * 0.3} ${y + s * 0.3} L${x} ${y + s} L${x - s * 0.3} ${y + s * 0.3} L${x - s} ${y} L${x - s * 0.3} ${y - s * 0.3} Z" fill="#fff"/>`).join('');
}

function dishTomatoEgg(w = 160, h = 120) {
  let b = plate(80, 66, 70, 44);
  b += `<path d="M28 66 C30 44 60 36 82 38 C108 40 132 50 132 68 C130 88 100 96 78 94 C52 92 28 84 28 66 Z" fill="#E8483B" opacity=".9"/>`;
  b += `<path d="M36 64 C40 50 64 44 82 46" stroke="#FF8A7A" stroke-width="3" fill="none" stroke-linecap="round" opacity=".8"/>`;
  b += eggChunk(56, 58, 1.15, -10) + tomatoWedge(82, 54, 1.1, 20) + eggChunk(104, 62, 1.1, 15) + tomatoWedge(62, 78, 1.0, -30) + eggChunk(86, 76, 1.2, 40) + tomatoWedge(110, 82, 0.9, 60) + eggChunk(44, 76, 0.9, 80);
  b += scallions([[70, 50, 20], [96, 50, -10], [76, 68, 45], [52, 68, -30], [100, 74, 10], [118, 66, 70], [66, 86, 0]]);
  b += shine([[60, 52, 3], [98, 58, 2.5], [84, 72, 2.2]]);
  return svg(w, h, '0 0 160 120', b);
}
function dishChoy(w = 160, h = 120) {
  let b = plate(80, 66, 70, 44);
  b += `<path d="M30 70 C40 56 120 54 130 70 C120 86 42 88 30 70Z" fill="#E9D6A0" opacity=".6"/>`;
  const stalk = (x, y, r) => `<g transform="translate(${x} ${y}) rotate(${r})"><rect x="-26" y="-3" width="40" height="6" rx="3" fill="#BFF0B5" stroke="${OL}" stroke-width="1.8"/><path d="M12 -2 C18 -14 34 -14 36 -2 C34 8 18 10 12 2Z" fill="#3FB86A" stroke="${OL}" stroke-width="1.8"/><path d="M16 -1 L32 -3" stroke="#7FE0A0" stroke-width="1.6"/></g>`;
  b += stalk(62, 58, -8) + stalk(80, 66, 4) + stalk(66, 76, -2) + stalk(92, 56, 12) + stalk(84, 80, -10);
  b += [[60, 64], [74, 60], [90, 70], [70, 82], [100, 76], [56, 74]].map(([x, y]) => `<circle cx="${x}" cy="${y}" r="2.2" fill="#FFF6D8" stroke="${OL}" stroke-width=".8"/>`).join('');
  b += shine([[72, 56, 3], [98, 66, 2.4]]);
  return svg(w, h, '0 0 160 120', b);
}
function dishFish(w = 160, h = 120) {
  let b = `<ellipse cx="80" cy="71" rx="76" ry="38" fill="#000" opacity=".22"/><ellipse cx="80" cy="66" rx="74" ry="38" fill="#fff" stroke="${OL}" stroke-width="2.4"/><ellipse cx="80" cy="66" rx="60" ry="28" fill="#E9EEF6"/>`;
  b += `<path d="M28 66 C40 52 100 46 116 64 C100 84 40 82 28 66Z" fill="#8A4E2A" opacity=".55"/>`;
  b += `<path d="M34 66 C46 48 98 46 112 64 C98 84 46 84 34 66Z" fill="#F4E7D8" stroke="${OL}" stroke-width="2"/><path d="M112 64 L132 52 L128 66 L132 80 Z" fill="#F4E7D8" stroke="${OL}" stroke-width="2"/><circle cx="46" cy="62" r="3" fill="${OL}"/><circle cx="45" cy="61" r="1" fill="#fff"/>`;
  b += [62, 76, 90].map(x => `<path d="M${x} 56 C${x + 4} 62 ${x + 4} 70 ${x} 76" stroke="#D9C2AA" stroke-width="2" fill="none"/>`).join('');
  b += [[56, 58, 30], [70, 62, -20], [84, 56, 40], [96, 64, -30], [66, 70, 10]].map(([x, y, r]) => `<rect x="${x}" y="${y}" width="12" height="2.4" rx="1.2" fill="#FFE39A" stroke="${OL}" stroke-width=".8" transform="rotate(${r} ${x} ${y})"/>`).join('');
  b += scallions([[60, 66, 10], [78, 58, -30], [92, 70, 20], [72, 74, 60]]);
  b += shine([[70, 54, 3], [100, 60, 2.5]]);
  return svg(w, h, '0 0 160 120', b);
}
function dishSoup(w = 160, h = 120) {
  let b = `<ellipse cx="80" cy="96" rx="52" ry="10" fill="#000" opacity=".22"/><path d="M24 58 C26 92 52 104 80 104 C108 104 134 92 136 58Z" fill="#FFFFFF" stroke="${OL}" stroke-width="2.4"/><path d="M40 92 C56 100 104 100 120 92" stroke="#6CB8F0" stroke-width="3" fill="none"/><ellipse cx="80" cy="58" rx="56" ry="18" fill="#fff" stroke="${OL}" stroke-width="2.4"/><ellipse cx="80" cy="59" rx="50" ry="14" fill="#F7C46A"/>`;
  b += `<path d="M48 58 C56 52 66 62 74 56 C82 50 92 62 100 56" stroke="#FFF3B5" stroke-width="4" fill="none" stroke-linecap="round"/>` + tomatoWedge(92, 62, .8, 30) + tomatoWedge(62, 62, .7, -20);
  b += scallions([[70, 52, 10], [86, 54, -20], [104, 60, 40], [56, 56, 0]]);
  return svg(w, h, '0 0 160 120', b);
}
const DISH = { tomato: dishTomatoEgg, choy: dishChoy, fish: dishFish, soup: dishSoup };

// 食材小图标（带小脸，可爱方案）
function face(x, y, s = 1) {
  return `<g transform="translate(${x} ${y}) scale(${s})"><circle cx="-5" cy="0" r="1.8" fill="${OL}"/><circle cx="5" cy="0" r="1.8" fill="${OL}"/><path d="M-2.5 3 Q0 5.5 2.5 3" stroke="${OL}" stroke-width="1.4" fill="none" stroke-linecap="round"/><ellipse cx="-8.5" cy="3.5" rx="2.2" ry="1.3" fill="#FF8FA0" opacity=".8"/><ellipse cx="8.5" cy="3.5" rx="2.2" ry="1.3" fill="#FF8FA0" opacity=".8"/></g>`;
}
const ING = {
  egg: () => `<ellipse cx="20" cy="22" rx="12" ry="14" fill="#FFF4E2" stroke="${OL}" stroke-width="2.2"/><path d="M13 14 C15 11 18 10 20 10" stroke="#fff" stroke-width="2.4" fill="none" stroke-linecap="round"/>${face(20, 24, .85)}`,
  tomato: () => `<circle cx="20" cy="23" r="13" fill="#FF5A4C" stroke="${OL}" stroke-width="2.2"/><path d="M14 12 L20 15 L26 12 L23 17 L17 17Z" fill="#3FB86A" stroke="${OL}" stroke-width="1.6" stroke-linejoin="round"/><path d="M11 20 C12 17 14 15 16 15" stroke="#FFB0A4" stroke-width="2.2" fill="none" stroke-linecap="round"/>${face(20, 25, .85)}`,
  rib: () => `<rect x="9" y="12" width="22" height="18" rx="7" fill="#E9877A" stroke="${OL}" stroke-width="2.2"/><rect x="5" y="17" width="30" height="5" rx="2.5" fill="#FFF3E0" stroke="${OL}" stroke-width="1.8"/>${face(20, 26, .75)}`,
  choy: () => `<path d="M17 34 L19 18 L21 34Z" fill="#CFF5C2" stroke="${OL}" stroke-width="1.8"/><path d="M19 20 C8 18 6 6 14 4 C20 3 22 10 19 20Z" fill="#3FB86A" stroke="${OL}" stroke-width="2"/><path d="M20 20 C30 18 34 8 27 5 C21 4 19 12 20 20Z" fill="#52CC7C" stroke="${OL}" stroke-width="2"/><circle cx="20" cy="6" r="2.4" fill="#FFE34D" stroke="${OL}" stroke-width="1"/>${face(20, 28, .6)}`,
  fish: () => `<path d="M6 21 C12 11 26 11 31 21 C26 31 12 31 6 21Z" fill="#9FD3F5" stroke="${OL}" stroke-width="2.2"/><path d="M30 21 L37 15 L36 21 L37 27Z" fill="#9FD3F5" stroke="${OL}" stroke-width="2" stroke-linejoin="round"/><path d="M10 18 C13 15 17 14 20 14" stroke="#fff" stroke-width="2" fill="none" stroke-linecap="round"/>${face(17, 22, .7)}`,
  tofu: () => `<path d="M8 16 L20 10 L32 16 L32 28 L20 34 L8 28Z" fill="#FFF7E6" stroke="${OL}" stroke-width="2.2" stroke-linejoin="round"/><path d="M8 16 L20 22 L32 16 M20 22 L20 34" stroke="${OL}" stroke-width="1.6" fill="none"/>${face(26, 25, .5)}`,
  potato: () => `<path d="M8 22 C8 12 18 9 26 11 C34 14 34 26 28 31 C20 36 8 32 8 22Z" fill="#E2B56E" stroke="${OL}" stroke-width="2.2"/><circle cx="14" cy="17" r="1.2" fill="#B07D3A"/><circle cx="28" cy="27" r="1.2" fill="#B07D3A"/>${face(20, 23, .8)}`,
  carrot: () => `<path d="M14 12 L28 14 L18 36Z" fill="#FF9A3D" stroke="${OL}" stroke-width="2.2" stroke-linejoin="round"/><path d="M18 12 C14 4 18 3 21 9 C22 3 27 3 24 12" fill="#3FB86A" stroke="${OL}" stroke-width="1.8"/>${face(20, 19, .55)}`,
  shiitake: () => `<path d="M6 20 C6 8 34 8 34 20 Z" fill="#9A5B3A" stroke="${OL}" stroke-width="2.2"/><rect x="16" y="20" width="8" height="12" rx="3" fill="#F3E2C8" stroke="${OL}" stroke-width="2"/><circle cx="14" cy="15" r="1.6" fill="#D9A27A"/><circle cx="25" cy="13" r="1.4" fill="#D9A27A"/>${face(20, 25, .45)}`,
  garlic: () => `<path d="M20 6 C22 12 32 16 32 25 C32 33 26 35 20 35 C14 35 8 33 8 25 C8 16 18 12 20 6Z" fill="#FFFBF2" stroke="${OL}" stroke-width="2.2"/><path d="M20 14 L20 34 M14 20 C13 26 14 31 16 34 M26 20 C27 26 26 31 24 34" stroke="#E3D6C2" stroke-width="1.4" fill="none"/>${face(20, 26, .6)}`,
  wing: () => `<path d="M8 26 C6 14 20 8 30 14 C36 18 34 28 26 30 C18 32 10 32 8 26Z" fill="#F5C38A" stroke="${OL}" stroke-width="2.2"/>${face(20, 22, .75)}`,
  pepper: () => `<path d="M12 14 C20 12 30 14 30 24 C30 32 22 36 18 34 C10 32 8 20 12 14Z" fill="#4FC06E" stroke="${OL}" stroke-width="2.2"/><path d="M20 13 L21 6" stroke="${OL}" stroke-width="2.4" stroke-linecap="round"/>${face(20, 24, .7)}`,
};
const ingIcon = (k, s = 40) => svg(s, s, '0 0 40 40', ING[k]());

// 小图标：火候、做法
const ICON = {
  fire: c => `<path d="M12 3 C14 8 19 9 19 15 C19 19 16 22 12 22 C8 22 5 19 5 15 C5 11 8 10 9 6 C10 9 11 10 12 3Z" fill="${c}" stroke="${OL}" stroke-width="1.8" stroke-linejoin="round"/><path d="M12 12 C13 14 15 15 15 17 C15 19 13.5 20 12 20 C10.5 20 9 19 9 17 C9 15.5 11 15 12 12Z" fill="#FFE07A"/>`,
  wok: c => `<path d="M3 11 L21 11 C20 18 16 20 12 20 C8 20 4 18 3 11Z" fill="#3B3346" stroke="${OL}" stroke-width="1.8"/><path d="M21 12 L23 10" stroke="${c}" stroke-width="2.4" stroke-linecap="round"/><path d="M8 8 C8 6 10 6 10 4 M13 8 C13 6 15 6 15 4" stroke="#fff" stroke-width="1.6" fill="none" stroke-linecap="round"/>`,
  timer: c => `<circle cx="12" cy="13" r="8" fill="#fff" stroke="${OL}" stroke-width="1.8"/><path d="M12 13 L12 8 M12 13 L15 15" stroke="${c}" stroke-width="2" stroke-linecap="round"/><rect x="10" y="2" width="4" height="3" rx="1" fill="${OL}"/>`,
  fridge: c => `<rect x="5" y="2" width="14" height="20" rx="3" fill="#fff" stroke="${OL}" stroke-width="1.8"/><path d="M5 9 L19 9" stroke="${OL}" stroke-width="1.6"/><path d="M8 5 L8 7 M8 12 L8 16" stroke="${c}" stroke-width="2" stroke-linecap="round"/>`,
  basket: c => `<path d="M3 10 L21 10 L18 21 L6 21Z" fill="${c}" stroke="${OL}" stroke-width="1.8" stroke-linejoin="round"/><path d="M7 10 C7 4 17 4 17 10" stroke="${OL}" stroke-width="1.8" fill="none"/><path d="M9 13 L9 18 M12 13 L12 18 M15 13 L15 18" stroke="${OL}" stroke-width="1.3"/>`,
  book: c => `<path d="M3 5 C7 3 10 4 12 6 C14 4 17 3 21 5 L21 20 C17 18 14 19 12 21 C10 19 7 18 3 20Z" fill="#fff" stroke="${OL}" stroke-width="1.8" stroke-linejoin="round"/><path d="M12 6 L12 21" stroke="${OL}" stroke-width="1.5"/><path d="M6 8 L9 8 M15 8 L18 8" stroke="${c}" stroke-width="2" stroke-linecap="round"/>`,
  gear: c => `<circle cx="12" cy="12" r="7" fill="${c}" stroke="${OL}" stroke-width="1.8"/><circle cx="12" cy="12" r="2.6" fill="#fff" stroke="${OL}" stroke-width="1.4"/>`,
  moon: c => `<path d="M15 3 C10 4 7 8 7 12 C7 17 11 21 16 21 C18 21 20 20 21 19 C15 19 11 15 11 10 C11 7 13 4 15 3Z" fill="${c}"/>`,
  lantern: c => `<rect x="10" y="1" width="4" height="3" fill="${OL}"/><ellipse cx="12" cy="12" rx="7" ry="8" fill="${c}" stroke="${OL}" stroke-width="1.6"/><path d="M12 4 L12 20 M7 12 L17 12" stroke="#fff" stroke-width="1" opacity=".5"/><rect x="10" y="20" width="4" height="3" fill="${OL}"/>`,
  stir: c => `<path d="M4 18 L14 8" stroke="${OL}" stroke-width="3.2" stroke-linecap="round"/><path d="M4 18 L14 8" stroke="${c}" stroke-width="1.6" stroke-linecap="round"/><ellipse cx="17" cy="6" rx="4" ry="3" fill="#fff" stroke="${OL}" stroke-width="1.6" transform="rotate(-45 17 6)"/>`,
};
const icon = (k, c = '#FFB45E', s = 24) => svg(s, s, '0 0 24 24', ICON[k](c));

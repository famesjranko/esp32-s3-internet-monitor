// Captures the preview pages with Playwright. Usage: node shots.mjs <dashboardUrl> <portalUrl> <outDir>
// Playwright resolves from PLAYWRIGHT_DIR (a node_modules parent), see shots.sh.
import { createRequire } from 'node:module';
import path from 'node:path';

const [dashboardUrl, portalUrl, outDir] = process.argv.slice(2);
const require = createRequire(path.join(process.env.PLAYWRIGHT_DIR, 'noop.js'));
const { chromium } = require('playwright');

const sizes = {
  desktop: { width: 1280, height: 800 },
  phone: { width: 390, height: 844 },
};

const browser = await chromium.launch();
for (const [name, viewport] of Object.entries(sizes)) {
  const ctx = await browser.newContext({
    viewport,
    deviceScaleFactor: name === 'phone' ? 2 : 1,
    isMobile: name === 'phone',
  });
  const page = await ctx.newPage();
  const shot = (file, fullPage = false) =>
    page.screenshot({ path: path.join(outDir, `${file}-${name}.png`), fullPage });

  // Login page, then sign in through the page's own form and the firmware's handleLogin.
  await page.goto(dashboardUrl);
  await page.waitForSelector('input[type=password]');
  await shot('login');
  await page.fill('input[type=password]', 'admin');
  await Promise.all([page.waitForURL(dashboardUrl + '**'), page.keyboard.press('Enter')]).catch(() => {});
  await page.waitForSelector('#mqttT', { timeout: 10000 });

  await shot('dashboard-top');
  await shot('dashboard-full', true);

  await page.click('#mqttT');
  await page.waitForTimeout(800); // the tab fetches /mqtt/config
  await page.locator('#mqttT').scrollIntoViewIfNeeded();
  await shot('mqtt-tab', true);

  await page.goto(portalUrl);
  await page.waitForSelector('.network');
  await shot('portal', true);
  await ctx.close();
}
await browser.close();

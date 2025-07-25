import { ArrowRight, CheckCircle, Cpu, ExternalLink, HardDrive, MemoryStick, Zap } from 'lucide-react';
import type { Metadata } from 'next';
import Link from 'next/link';

import { Badge } from '@/components/ui/badge';
import { Button } from '@/components/ui/button';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Separator } from '@/components/ui/separator';

export const metadata: Metadata = {
  title: 'Boards',
  description: 'Supported boards and their details',
};

const sensors = [
  'DHT22 - Temperature/Humidity',
  'BME280 - Environmental sensor',
  'Soil moisture sensors',
  'pH sensors',
  'Light sensors',

  // 'Industrial temperature sensors',
  // 'Precision humidity sensors',
  // 'Professional pH meters',
  // 'High-accuracy pressure sensors',

  // 'Low-power temperature sensors',
  // 'Battery-optimized humidity sensors',
  // 'Ultra-low power soil sensors',
  // 'Energy-harvesting compatible sensors',
];

const boards = [
  {
    id: 'esp32s3_devkitc',
    name: 'ESP32-S3 DevKitC-1',
    manufacturer: 'Espressif Systems',
    description: 'Powerful dual-core microcontroller with advanced connectivity and AI acceleration',
    status: 'Primary Platform',
    price: '$8-12',
    availability: 'Widely Available',
    specifications: {
      cpu: 'Dual-core Xtensa LX7 @ 240MHz',
      memory: '512KB SRAM + 384KB ROM',
      flash: '8MB (configurable)',
      connectivity: ['WiFi 802.11 b/g/n', 'Bluetooth 5.0 LE', 'USB OTG'],
      gpio: '45 programmable GPIOs',
      adc: '2x 12-bit SAR ADCs',
      interfacesList: ['SPI', 'I2C', 'I2S', 'UART', 'CAN', 'USB'],
      power: '3.3V, 500mA typical',
      dimensions: '55.0 × 26.0 mm',
    },
    features: [
      'AI acceleration support',
      'Advanced security features',
      'Low power consumption',
      'Rich peripheral set',
      'USB native support',
      'Vector instructions',
    ],
    useCases: ['Smart agriculture sensors', 'Environmental monitoring', 'Edge AI applications', 'IoT gateways'],
    frameworks: ['Arduino', 'ESP-IDF', 'Zephyr'],
    applications: {
      keeper: 'Environmental control with WiFi connectivity and sensor fusion',
      pot: 'Individual plant monitoring with low-power wireless communication',
    },
    documentation: 'https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/',
    datasheet: 'https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf',
  },
  {
    id: 'esp32c6_devkitc',
    name: 'ESP32-C6 DevKitC-1',
    manufacturer: 'Espressif Systems',
    description: 'Efficient RISC-V microcontroller with WiFi 6, Bluetooth 5 LE, and 802.15.4 connectivity',
    status: 'Experimental',
    price: '$10-15',
    availability: 'Available',
    specifications: {
      cpu: 'Dual-core RISC-V @ 160 MHz',
      memory: '512KB SRAM + 320KB ROM',
      flash: '8MB (configurable)',
      connectivity: [
        'Wi-Fi 6 (802.11 b/g/n/ax, 2.4 GHz)',
        'Bluetooth 5.3 LE',
        'IEEE 802.15.4 (Zigbee 3.0, Thread 1.3)',
        'USB 2.0 Full Speed',
      ],
      gpio: '30 programmable GPIOs',
      adc: '12-bit SAR ADCs',
      interfacesList: ['SPI', 'I2C', 'I2S', 'UART', 'CAN', 'USB'],
      power: '3.3V, 500mA typical',
      dimensions: '51.8 × 25.4 mm',
    },
    features: [
      '802.11ax Wi-Fi 6 support',
      'Bluetooth 5.3 LE',
      'IEEE 802.15.4 (Thread & Zigbee)',
      'Hardware security (Secure Boot, Flash Encryption)',
      'USB Serial/JTAG interface',
      'Low-power sensor and sleep modes',
    ],
    useCases: [
      'Thread/Zigbee border router',
      'IoT gateway',
      'Smart home hub',
      'Wireless sensor networks',
      'Industrial automation node',
    ],
    frameworks: ['Arduino', 'ESP-IDF', 'Zephyr'],
    applications: {
      keeper: 'Environmental control with WiFi connectivity and sensor fusion',
      pot: 'Individual plant monitoring with low-power wireless communication',
    },
    documentation: 'https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/',
    datasheet: 'https://www.espressif.com/sites/default/files/documentation/esp32-c6_datasheet_en.pdf',
  },
  {
    id: 'frdm_rw612',
    name: 'FRDM-RW612',
    manufacturer: 'NXP Semiconductors',
    description: 'Advanced development board with tri-radio wireless connectivity and EdgeLock security',
    status: 'Experimental',
    price: '$25-35',
    availability: 'Relatively New',
    specifications: {
      cpu: 'Dual-core Cortex-M33 @ 260MHz',
      memory: '1.2MB SRAM + 256KB ROM',
      flash: '4MB internal + external support',
      connectivity: ['WiFi 6', 'Bluetooth 5.3 LE', '802.15.4/Thread'],
      gpio: '56 GPIOs',
      adc: '16-bit SAR ADC',
      interfacesList: ['SPI', 'I2C', 'I2S', 'UART', 'USB', 'CAN-FD'],
      power: '3.3V, advanced power management',
      dimensions: '82.5 × 57.2 mm',
    },
    features: [
      'Tri-radio wireless',
      'EdgeLock security enclave',
      'Advanced power management',
      'High-performance ARM cores',
      'Thread/Matter support',
      'Hardware crypto acceleration',
    ],
    useCases: [
      'Industrial IoT applications',
      'Secure edge computing',
      'Smart building systems',
      'Professional development',
    ],
    frameworks: ['Zephyr'],
    applications: {
      keeper: 'Advanced environmental control with Thread mesh networking and security',
      pot: 'Professional-grade plant monitoring with encrypted data transmission',
    },
    documentation: 'https://www.nxp.com/design/development-boards/freedom-development-boards/',
    datasheet: 'https://www.nxp.com/docs/en/data-sheet/RW612.pdf',
  },
  {
    id: 'nrf7002dk',
    name: 'nRF7002 DK',
    manufacturer: 'Nordic Semiconductor',
    description: 'Ultra-low power development kit with WiFi 6 and advanced power management',
    status: 'Experimental',
    price: '$50-70',
    availability: 'Available',
    specifications: {
      cpu: 'ARM Cortex-M33 @ 128MHz',
      memory: '512KB RAM + 1MB Flash',
      flash: '1MB internal + external support',
      connectivity: ['WiFi 6', 'Bluetooth 5.3 LE', 'NFC'],
      gpio: '48 GPIOs',
      adc: '12-bit ADC',
      interfacesList: ['SPI', 'I2C', 'UART', 'USB', 'QSPI'],
      power: 'Ultra-low power design',
      dimensions: '100 × 60 mm',
    },
    features: [
      'Ultra-low power WiFi',
      'Advanced power profiling',
      'Bluetooth mesh support',
      'NFC connectivity',
      'Hardware crypto',
      'Real-time debugging',
    ],
    useCases: ['Battery-powered sensors', 'Long-term monitoring', 'Wearable devices', 'Remote sensing applications'],
    frameworks: ['Zephyr'],
    applications: {
      keeper: 'Long-term environmental monitoring with months of battery life',
      pot: 'Ultra-low power plant monitoring for remote or battery-powered installations',
    },
    documentation: 'https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/',
    datasheet: 'https://infocenter.nordicsemi.com/pdf/nRF7002_PS_v1.0.pdf',
  },
];

export default async function Page() {
  return (
    <div className="bg-background min-h-screen">
      {/* Hero Section */}
      <section className="to-background relative overflow-hidden bg-gradient-to-b from-green-50 py-20">
        <div className="container mx-auto px-6">
          <div className="mx-auto max-w-4xl text-center">
            <h1 className="mb-6 text-4xl font-bold tracking-tight text-gray-900 sm:text-5xl">
              Supported Development
              <span className="text-green-600"> Boards</span>
            </h1>
            <p className="mb-8 text-xl leading-relaxed text-gray-600">
              Professional-grade development boards chosen for their reliability, performance, and ecosystem support in
              IoT applications.
            </p>
            <div className="mb-8 flex flex-wrap justify-center gap-2">
              <Badge variant="secondary">4 Boards Supported</Badge>
              <Badge variant="secondary">Multi-Framework</Badge>
              <Badge variant="secondary">Production Ready</Badge>
              <Badge variant="secondary">Professional Grade</Badge>
            </div>
          </div>
        </div>
      </section>

      {/* Board Comparison */}
      <section className="bg-gray-50 py-16">
        <div className="container mx-auto px-6">
          <div className="mb-12 text-center">
            <h2 className="mb-4 text-3xl font-bold text-gray-900">Board Comparison</h2>
            <p className="text-xl text-gray-600">Technical specifications at a glance</p>
          </div>

          <div className="overflow-x-auto">
            <table className="w-full rounded-lg bg-white shadow-sm">
              <thead className="bg-gray-50">
                <tr>
                  <th className="px-6 py-4 text-left text-sm font-medium text-gray-500">Specification</th>
                  <th className="px-6 py-4 text-center text-sm font-medium text-blue-600">ESP32-S3 DEVKIT-C</th>
                  <th className="px-6 py-4 text-center text-sm font-medium text-blue-600">ESP32-C6 DEVKIT-C</th>
                  <th className="px-6 py-4 text-center text-sm font-medium text-purple-600">FRDM-RW612</th>
                  <th className="px-6 py-4 text-center text-sm font-medium text-green-600">nRF7002 DK</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-gray-200">
                <tr>
                  <td className="px-6 py-4 text-sm font-medium text-gray-900">CPU Performance</td>
                  <td className="px-6 py-4 text-center text-sm text-gray-600">240MHz Dual-core</td>
                  <td className="px-6 py-4 text-center text-sm text-gray-600">160MHz Dual-core</td>
                  <td className="px-6 py-4 text-center text-sm text-gray-600">260MHz Dual-core</td>
                  <td className="px-6 py-4 text-center text-sm text-gray-600">128MHz Single-core</td>
                </tr>
                <tr className="bg-gray-50">
                  <td className="px-6 py-4 text-sm font-medium text-gray-900">Flash</td>
                  <td className="px-6 py-4 text-center text-sm text-gray-600">384KB ROM + 8MB QSPI</td>
                  <td className="px-6 py-4 text-center text-sm text-gray-600">320KB ROM + 8MB QSPI</td>
                  <td className="px-6 py-4 text-center text-sm text-gray-600">256KB ROM + 64MB QSPI</td>
                  <td className="px-6 py-4 text-center text-sm text-gray-600">1MB ROM</td>
                </tr>
                <tr className="bg-gray-50">
                  <td className="px-6 py-4 text-sm font-medium text-gray-900">Memory</td>
                  <td className="px-6 py-4 text-center text-sm text-gray-600">512KB SRAM + 4MB PSRAM</td>
                  <td className="px-6 py-4 text-center text-sm text-gray-600">512KB SRAM</td>
                  <td className="px-6 py-4 text-center text-sm text-gray-600">1.2MB SRAM + 8MB PSRAM</td>
                  <td className="px-6 py-4 text-center text-sm text-gray-600">512KB RAM</td>
                </tr>
                <tr>
                  <td className="px-6 py-4 text-sm font-medium text-gray-900">Wireless</td>
                  <td className="px-6 py-4 text-center">
                    <Badge variant="outline">WiFi + BLE</Badge>
                  </td>
                  <td className="px-6 py-4 text-center">
                    <Badge variant="default">WiFi 6 + BLE + Thread</Badge>
                  </td>
                  <td className="px-6 py-4 text-center">
                    <Badge variant="default">WiFi 6 + BLE + Thread</Badge>
                  </td>
                  <td className="px-6 py-4 text-center">
                    <Badge variant="secondary">WiFi 6 + BLE</Badge>
                  </td>
                </tr>
                <tr className="bg-gray-50">
                  <td className="px-6 py-4 text-sm font-medium text-gray-900">Power Efficiency</td>
                  <td className="px-6 py-4 text-center">
                    <Badge variant="secondary">Good</Badge>
                  </td>
                  <td className="px-6 py-4 text-center">
                    <Badge variant="secondary">Good</Badge>
                  </td>
                  <td className="px-6 py-4 text-center">
                    <Badge variant="secondary">Good</Badge>
                  </td>
                  <td className="px-6 py-4 text-center">
                    <Badge variant="default">Excellent</Badge>
                  </td>
                </tr>
                <tr>
                  <td className="px-6 py-4 text-sm font-medium text-gray-900">Price Range</td>
                  <td className="px-6 py-4 text-center text-sm text-gray-600">$8-12</td>
                  <td className="px-6 py-4 text-center text-sm text-gray-600">$10-15</td>
                  <td className="px-6 py-4 text-center text-sm text-gray-600">$25-35</td>
                  <td className="px-6 py-4 text-center text-sm text-gray-600">$50-70</td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>
      </section>

      {/* Detailed Board Information */}
      <section className="py-20">
        <div className="container mx-auto px-6">
          <div className="space-y-20">
            {boards.map((board, index) => (
              <div key={board.id} className="mx-auto max-w-6xl" id={board.id}>
                <div className="grid items-start gap-12 lg:grid-cols-2">
                  {/* Board Overview */}
                  <div className={index % 2 === 1 ? 'lg:order-2' : ''}>
                    <div className="mb-6 flex items-center space-x-3">
                      <div className="rounded-lg bg-gray-100 p-3">
                        <Cpu className="h-6 w-6 text-gray-700" />
                      </div>
                      <div>
                        <h3 className="text-2xl font-bold text-gray-900">{board.name}</h3>
                        <p className="text-sm text-gray-600">{board.manufacturer}</p>
                      </div>
                      <Badge variant={board.status === 'Primary Platform' ? 'default' : 'secondary'}>
                        {board.status}
                      </Badge>
                    </div>

                    <p className="mb-6 text-lg text-gray-600">{board.description}</p>

                    <div className="mb-6 grid grid-cols-2 gap-4">
                      <div>
                        <h4 className="mb-2 font-semibold text-gray-900">Price Range</h4>
                        <Badge variant="outline">{board.price}</Badge>
                      </div>
                      <div>
                        <h4 className="mb-2 font-semibold text-gray-900">Availability</h4>
                        <Badge variant="secondary">{board.availability}</Badge>
                      </div>
                    </div>

                    <div className="space-y-4">
                      <div>
                        <h4 className="mb-2 font-semibold text-gray-900">Key Features</h4>
                        <div className="grid grid-cols-2 gap-2">
                          {board.features.map((feature, idx) => (
                            <div key={idx} className="flex items-center space-x-2">
                              <CheckCircle className="h-4 w-4 text-green-500" />
                              <span className="text-sm text-gray-600">{feature}</span>
                            </div>
                          ))}
                        </div>
                      </div>
                    </div>

                    <div className="mt-6 flex space-x-4">
                      <Button variant="outline" size="sm" asChild>
                        <a href={board.documentation} target="_blank" rel="noopener noreferrer">
                          <ExternalLink className="mr-2 h-4 w-4" />
                          Documentation
                        </a>
                      </Button>
                      <Button variant="outline" size="sm" asChild>
                        <a href={board.datasheet} target="_blank" rel="noopener noreferrer">
                          <ExternalLink className="mr-2 h-4 w-4" />
                          Datasheet
                        </a>
                      </Button>
                    </div>
                  </div>

                  {/* Board Image and Specs */}
                  <div className={index % 2 === 1 ? 'lg:order-1' : ''}>
                    <Card>
                      <CardHeader>
                        <CardTitle>Technical Specifications</CardTitle>
                      </CardHeader>
                      <CardContent className="space-y-4">
                        <div className="grid grid-cols-2 gap-4 text-sm">
                          <div className="flex items-center space-x-2">
                            <Cpu className="h-4 w-4 text-gray-500" />
                            <div>
                              <p className="font-medium">CPU</p>
                              <p className="text-gray-600">{board.specifications.cpu}</p>
                            </div>
                          </div>
                          <div className="flex items-center space-x-2">
                            <MemoryStick className="h-4 w-4 text-gray-500" />
                            <div>
                              <p className="font-medium">Memory</p>
                              <p className="text-gray-600">{board.specifications.memory}</p>
                            </div>
                          </div>
                          <div className="flex items-center space-x-2">
                            <HardDrive className="h-4 w-4 text-gray-500" />
                            <div>
                              <p className="font-medium">Flash</p>
                              <p className="text-gray-600">{board.specifications.flash}</p>
                            </div>
                          </div>
                          <div className="flex items-center space-x-2">
                            <Zap className="h-4 w-4 text-gray-500" />
                            <div>
                              <p className="font-medium">Power</p>
                              <p className="text-gray-600">{board.specifications.power}</p>
                            </div>
                          </div>
                        </div>
                        <Separator />
                        <div>
                          <p className="mb-2 font-medium">Connectivity</p>
                          <div className="flex flex-wrap gap-2">
                            {board.specifications.connectivity.map((conn, idx) => (
                              <Badge key={idx} variant="outline" className="text-xs">
                                {conn}
                              </Badge>
                            ))}
                          </div>
                        </div>
                        <div>
                          <p className="mb-2 font-medium">Interfaces</p>
                          <div className="flex flex-wrap gap-2">
                            {board.specifications.interfacesList.map((interfaceItem, idx) => (
                              <Badge key={idx} variant="secondary" className="text-xs">
                                {interfaceItem}
                              </Badge>
                            ))}
                          </div>
                        </div>
                      </CardContent>
                    </Card>
                  </div>
                </div>

                {/* Application-Specific Information */}
                <div className="mt-12 grid gap-6 md:grid-cols-2 lg:grid-cols-4">
                  <Card>
                    <CardHeader>
                      <CardTitle className="text-lg">Use Cases</CardTitle>
                    </CardHeader>
                    <CardContent>
                      <ul className="space-y-2">
                        {board.useCases.map((useCase, idx) => (
                          <li key={idx} className="flex items-center space-x-2 text-sm text-gray-600">
                            <ArrowRight className="h-3 w-3" />
                            <span>{useCase}</span>
                          </li>
                        ))}
                      </ul>
                    </CardContent>
                  </Card>

                  <Card>
                    <CardHeader>
                      <CardTitle className="text-lg">Supported Frameworks</CardTitle>
                    </CardHeader>
                    <CardContent>
                      <div className="space-y-2">
                        {board.frameworks.map((framework, idx) => (
                          <Badge key={idx} variant="outline" className="mr-2">
                            {framework}
                          </Badge>
                        ))}
                      </div>
                    </CardContent>
                  </Card>

                  <Card>
                    <CardHeader>
                      <CardTitle className="text-lg">Compatible Sensors</CardTitle>
                    </CardHeader>
                    <CardContent>
                      <ul className="space-y-2">
                        {sensors.slice(0, 4).map((sensor, idx) => (
                          <li key={idx} className="text-sm text-gray-600">
                            <span className="font-medium">{sensor.split(' - ')[0]}</span>
                            <br />
                            <span className="text-xs text-gray-500">{sensor.split(' - ')[1]}</span>
                          </li>
                        ))}
                      </ul>
                    </CardContent>
                  </Card>

                  <Card>
                    <CardHeader>
                      <CardTitle className="text-lg">Korra Applications</CardTitle>
                    </CardHeader>
                    <CardContent className="space-y-3">
                      <div>
                        <p className="mb-1 text-sm font-medium text-gray-700">Keeper Devices:</p>
                        <p className="text-xs text-gray-600">{board.applications.keeper}</p>
                      </div>
                      <div>
                        <p className="mb-1 text-sm font-medium text-gray-700">Pot Devices:</p>
                        <p className="text-xs text-gray-600">{board.applications.pot}</p>
                      </div>
                    </CardContent>
                  </Card>
                </div>

                {index < boards.length - 1 && <Separator className="mt-16" />}
              </div>
            ))}
          </div>
        </div>
      </section>

      {/* CTA Section */}
      <section className="bg-green-600 py-20">
        <div className="container mx-auto px-6 text-center">
          <div className="mx-auto max-w-3xl">
            <h2 className="mb-4 text-3xl font-bold text-white">Ready to Build Your IoT Solution?</h2>
            <p className="mb-8 text-xl text-green-100">
              Choose the right development board for your project and start building with Korra&apos;s comprehensive
              platform.
            </p>
            <div className="flex flex-col justify-center gap-4 sm:flex-row">
              <Button size="lg" variant="secondary" asChild>
                <Link href="/dashboard/firmware">
                  Explore Firmware Options
                  <ArrowRight className="ml-2 h-4 w-4" />
                </Link>
              </Button>
              <Button
                size="lg"
                variant="outline"
                className="border-white bg-transparent text-white hover:bg-white hover:text-green-600"
                asChild
              >
                <Link href="/frameworks">
                  View Development Frameworks
                  <ExternalLink className="ml-2 h-4 w-4" />
                </Link>
              </Button>
            </div>
          </div>
        </div>
      </section>
    </div>
  );
}

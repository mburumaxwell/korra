import { ArrowRight, CheckCircle, Code2, ExternalLink, Layers, Settings, Terminal } from 'lucide-react';
import type { Metadata } from 'next';
import Link from 'next/link';

import { Badge } from '@/components/ui/badge';
import { Button } from '@/components/ui/button';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Separator } from '@/components/ui/separator';
import { samples } from './code-samples';

export const metadata: Metadata = {
  title: 'Frameworks',
  description: 'Supported frameworks and their details',
};

const frameworks = [
  {
    id: 'arduino',
    name: 'Arduino Framework',
    version: '3.2.1',
    description: 'Rapid prototyping platform with extensive library ecosystem',
    color: 'bg-blue-500',
    textColor: 'text-blue-600',
    icon: Code2,
    status: 'Production Ready',
    difficulty: 'Beginner',
    features: [
      'Simple C++ syntax',
      'Extensive library ecosystem',
      'Large community support',
      'Quick prototyping',
      'Cross-platform IDE',
      'Built-in examples',
    ],
    useCases: ['Educational projects', 'Rapid prototyping', 'Hobbyist applications', 'Simple IoT devices'],
    pros: [
      'Easy to learn and use',
      'Vast library collection',
      'Strong community',
      'Quick development cycle',
      'Good documentation',
    ],
    cons: [
      'Limited real-time capabilities',
      'Less memory efficient',
      'Basic debugging tools',
      'Monolithic architecture',
    ],
    codeExample: samples.arduino,
    libraries: [
      'WiFi - Network connectivity',
      'ArduinoJson - JSON parsing',
      'DHT sensor library - Temperature/humidity',
      'PubSubClient - MQTT communication',
      'HTTPClient - HTTP requests',
    ],
    buildTools: ['Arduino IDE', 'PlatformIO', 'Arduino CLI'],
    deployment: 'OTA updates via Arduino OTA library',
  },
  {
    id: 'zephyr',
    name: 'Zephyr RTOS',
    version: '4.1.0',
    description: 'Real-time operating system for connected, resource-constrained devices',
    color: 'bg-purple-500',
    textColor: 'text-purple-600',
    icon: Layers,
    status: 'Prototyping',
    difficulty: 'Advanced',
    features: [
      'Real-time kernel',
      'Memory protection',
      'Multi-threading',
      'Device tree configuration',
      'Modular architecture',
      'Power management',
    ],
    useCases: ['Industrial IoT', 'Real-time systems', 'Safety-critical applications', 'Low-power devices'],
    pros: [
      'True real-time performance',
      'Memory protection',
      'Scalable architecture',
      'Industry-standard RTOS',
      'Advanced power management',
    ],
    cons: ['Steep learning curve', 'Complex configuration', 'Longer development time', 'Limited Arduino compatibility'],
    codeExample: samples.zephyr,
    libraries: [
      'Networking - WiFi, Bluetooth, Thread',
      'Sensors - Unified sensor API',
      'Storage - Flash, EEPROM, SD card',
      'Crypto - Hardware crypto acceleration',
      'Power - Advanced power management',
    ],
    buildTools: ['West build system', 'CMake', 'Ninja', 'Device Tree Compiler'],
    deployment: 'MCUboot bootloader with signed image updates',
  },
  {
    id: 'espidf',
    name: 'ESP-IDF',
    version: '5.4.2',
    description: "Espressif's official development framework for ESP32 series",
    color: 'bg-green-500',
    textColor: 'text-green-600',
    icon: Terminal,
    status: 'Planned',
    difficulty: 'Intermediate',
    features: [
      'FreeRTOS integration',
      'Hardware abstraction',
      'Comprehensive APIs',
      'Performance optimization',
      'Security features',
      'Dual-core support',
    ],
    useCases: [
      'Commercial IoT products',
      'High-performance applications',
      'Custom hardware solutions',
      'Professional development',
    ],
    pros: [
      'Full hardware access',
      'Optimized performance',
      'Professional toolchain',
      'Comprehensive documentation',
      'Active development',
    ],
    cons: ['ESP32-specific only', 'Complex setup process', 'Requires C/C++ knowledge', 'Steeper learning curve'],
    codeExample: samples.espidf,
    libraries: [
      'WiFi - Advanced WiFi management',
      'Bluetooth - BLE and Classic support',
      'MQTT - Native MQTT client',
      'HTTP - HTTP/HTTPS client/server',
      'JSON - cJSON library integration',
    ],
    buildTools: ['ESP-IDF build system', 'CMake', 'Ninja', 'esptool.py'],
    deployment: 'ESP32 OTA with rollback and encryption support',
  },
];

export default async function Page() {
  return (
    <div className="bg-background min-h-screen">
      {/* Hero Section */}
      <section className="to-background relative overflow-hidden bg-gradient-to-b from-blue-50 py-20">
        <div className="container mx-auto px-6">
          <div className="mx-auto max-w-4xl text-center">
            <h1 className="mb-6 text-4xl font-bold tracking-tight text-gray-900 sm:text-5xl">
              Supported Development
              <span className="text-blue-600"> Frameworks</span>
            </h1>
            <p className="mb-8 text-xl leading-relaxed text-gray-600">
              Choose the right development framework for your IoT project. Each framework offers unique advantages for
              different use cases and skill levels.
            </p>
            <div className="mb-8 flex flex-wrap justify-center gap-2">
              <Badge variant="secondary">3 Frameworks Supported</Badge>
              <Badge variant="secondary">Production Ready</Badge>
              <Badge variant="secondary">OTA Updates</Badge>
              <Badge variant="secondary">Cloud Integration</Badge>
            </div>
          </div>
        </div>
      </section>

      {/* Framework Comparison */}
      <section className="bg-gray-50 py-16">
        <div className="container mx-auto px-6">
          <div className="mb-12 text-center">
            <h2 className="mb-4 text-3xl font-bold text-gray-900">Framework Comparison</h2>
            <p className="text-xl text-gray-600">Quick comparison to help you choose the right framework</p>
          </div>

          <div className="overflow-x-auto">
            <table className="w-full rounded-lg bg-white shadow-sm">
              <thead className="bg-gray-50">
                <tr>
                  <th className="px-6 py-4 text-left text-sm font-medium text-gray-500">Feature</th>
                  <th className="px-6 py-4 text-center text-sm font-medium text-blue-600">Arduino</th>
                  <th className="px-6 py-4 text-center text-sm font-medium text-purple-600">Zephyr</th>
                  <th className="px-6 py-4 text-center text-sm font-medium text-green-600">ESP-IDF</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-gray-200">
                <tr>
                  <td className="px-6 py-4 text-sm font-medium text-gray-900">Difficulty Level</td>
                  <td className="px-6 py-4 text-center text-sm text-gray-600">Beginner</td>
                  <td className="px-6 py-4 text-center text-sm text-gray-600">Advanced</td>
                  <td className="px-6 py-4 text-center text-sm text-gray-600">Intermediate</td>
                </tr>
                <tr className="bg-gray-50">
                  <td className="px-6 py-4 text-sm font-medium text-gray-900">Real-time Performance</td>
                  <td className="px-6 py-4 text-center">
                    <Badge variant="outline">Limited</Badge>
                  </td>
                  <td className="px-6 py-4 text-center">
                    <Badge variant="default">Excellent</Badge>
                  </td>
                  <td className="px-6 py-4 text-center">
                    <Badge variant="secondary">Good</Badge>
                  </td>
                </tr>
                <tr>
                  <td className="px-6 py-4 text-sm font-medium text-gray-900">Memory Efficiency</td>
                  <td className="px-6 py-4 text-center">
                    <Badge variant="outline">Basic</Badge>
                  </td>
                  <td className="px-6 py-4 text-center">
                    <Badge variant="default">Excellent</Badge>
                  </td>
                  <td className="px-6 py-4 text-center">
                    <Badge variant="default">Excellent</Badge>
                  </td>
                </tr>
                <tr className="bg-gray-50">
                  <td className="px-6 py-4 text-sm font-medium text-gray-900">Development Speed</td>
                  <td className="px-6 py-4 text-center">
                    <Badge variant="default">Fast</Badge>
                  </td>
                  <td className="px-6 py-4 text-center">
                    <Badge variant="outline">Slow</Badge>
                  </td>
                  <td className="px-6 py-4 text-center">
                    <Badge variant="secondary">Medium</Badge>
                  </td>
                </tr>
                <tr>
                  <td className="px-6 py-4 text-sm font-medium text-gray-900">Hardware Support</td>
                  <td className="px-6 py-4 text-center">
                    <Badge variant="default">Wide</Badge>
                  </td>
                  <td className="px-6 py-4 text-center">
                    <Badge variant="default">Wide</Badge>
                  </td>
                  <td className="px-6 py-4 text-center">
                    <Badge variant="outline">ESP32 Only</Badge>
                  </td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>
      </section>

      {/* Detailed Framework Information */}
      <section className="py-20">
        <div className="container mx-auto px-6">
          <div className="space-y-20">
            {frameworks.map((framework, index) => {
              const Icon = framework.icon;

              return (
                <div key={framework.id} className="mx-auto max-w-6xl" id={framework.id}>
                  <div className="grid items-start gap-12 lg:grid-cols-2">
                    {/* Framework Overview */}
                    <div className={index % 2 === 1 ? 'lg:order-2' : ''}>
                      <div className="mb-6 flex items-center space-x-3">
                        <div className={`rounded-lg p-3 ${framework.color}`}>
                          <Icon className="h-6 w-6 text-white" />
                        </div>
                        <div>
                          <h3 className="text-2xl font-bold text-gray-900">{framework.name}</h3>
                          <p className="text-sm text-gray-600">Version {framework.version}</p>
                        </div>
                        <Badge variant="outline">{framework.status}</Badge>
                      </div>

                      <p className="mb-6 text-lg text-gray-600">{framework.description}</p>

                      <div className="mb-6 grid grid-cols-2 gap-4">
                        <div>
                          <h4 className="mb-2 font-semibold text-gray-900">Difficulty</h4>
                          <Badge variant="secondary">{framework.difficulty}</Badge>
                        </div>
                        <div>
                          <h4 className="mb-2 font-semibold text-gray-900">Best For</h4>
                          <p className="text-sm text-gray-600">{framework.useCases[0]}</p>
                        </div>
                      </div>

                      <div className="space-y-4">
                        <div>
                          <h4 className="mb-2 font-semibold text-gray-900">Key Features</h4>
                          <div className="grid grid-cols-2 gap-2">
                            {framework.features.map((feature, idx) => (
                              <div key={idx} className="flex items-center space-x-2">
                                <CheckCircle className="h-4 w-4 text-green-500" />
                                <span className="text-sm text-gray-600">{feature}</span>
                              </div>
                            ))}
                          </div>
                        </div>
                      </div>
                    </div>

                    {/* Code Example */}
                    <div className={index % 2 === 1 ? 'lg:order-1' : ''}>
                      <Card>
                        <CardHeader>
                          <CardTitle className="flex items-center space-x-2">
                            <Code2 className="h-5 w-5" />
                            <span>Code Example</span>
                          </CardTitle>
                          <CardDescription>Basic sensor reading implementation</CardDescription>
                        </CardHeader>
                        <CardContent>
                          <pre className="overflow-x-auto rounded-lg bg-gray-900 p-4 text-xs text-gray-100">
                            <code>{framework.codeExample}</code>
                          </pre>
                        </CardContent>
                      </Card>
                    </div>
                  </div>

                  {/* Detailed Information Cards */}
                  <div className="mt-12 grid gap-6 md:grid-cols-2 lg:grid-cols-4">
                    <Card>
                      <CardHeader>
                        <CardTitle className="text-lg">Use Cases</CardTitle>
                      </CardHeader>
                      <CardContent>
                        <ul className="space-y-2">
                          {framework.useCases.map((useCase, idx) => (
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
                        <CardTitle className="text-lg">Advantages</CardTitle>
                      </CardHeader>
                      <CardContent>
                        <ul className="space-y-2">
                          {framework.pros.slice(0, 4).map((pro, idx) => (
                            <li key={idx} className="flex items-center space-x-2 text-sm text-gray-600">
                              <CheckCircle className="h-3 w-3 text-green-500" />
                              <span>{pro}</span>
                            </li>
                          ))}
                        </ul>
                      </CardContent>
                    </Card>

                    <Card>
                      <CardHeader>
                        <CardTitle className="text-lg">Key Libraries</CardTitle>
                      </CardHeader>
                      <CardContent>
                        <ul className="space-y-2">
                          {framework.libraries.slice(0, 4).map((library, idx) => (
                            <li key={idx} className="text-sm text-gray-600">
                              <span className="font-medium">{library.split(' - ')[0]}</span>
                              <br />
                              <span className="text-xs text-gray-500">{library.split(' - ')[1]}</span>
                            </li>
                          ))}
                        </ul>
                      </CardContent>
                    </Card>

                    <Card>
                      <CardHeader>
                        <CardTitle className="text-lg">Build Tools</CardTitle>
                      </CardHeader>
                      <CardContent>
                        <ul className="space-y-2">
                          {framework.buildTools.map((tool, idx) => (
                            <li key={idx} className="flex items-center space-x-2 text-sm text-gray-600">
                              <Settings className="h-3 w-3" />
                              <span>{tool}</span>
                            </li>
                          ))}
                        </ul>
                        <Separator className="my-3" />
                        <div>
                          <p className="mb-1 text-xs font-medium text-gray-700">OTA Deployment:</p>
                          <p className="text-xs text-gray-600">{framework.deployment}</p>
                        </div>
                      </CardContent>
                    </Card>
                  </div>

                  {index < frameworks.length - 1 && <Separator className="mt-16" />}
                </div>
              );
            })}
          </div>
        </div>
      </section>

      {/* CTA Section */}
      <section className="bg-blue-600 py-20">
        <div className="container mx-auto px-6 text-center">
          <div className="mx-auto max-w-3xl">
            <h2 className="mb-4 text-3xl font-bold text-white">Ready to Start Development?</h2>
            <p className="mb-8 text-xl text-blue-100">
              Choose your framework and start building your next IoT project with Korra&apos;s comprehensive platform.
            </p>
            <div className="flex flex-col justify-center gap-4 sm:flex-row">
              <Button size="lg" variant="secondary" asChild>
                <Link href="/dashboard/firmware">
                  View Firmware Management
                  <ArrowRight className="ml-2 h-4 w-4" />
                </Link>
              </Button>
              <Button
                size="lg"
                variant="outline"
                className="border-white bg-transparent text-white hover:bg-white hover:text-blue-600"
                asChild
              >
                <Link href="/boards">
                  Explore Supported Boards
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

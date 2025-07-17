import {
  ArrowRight,
  BarChart3,
  Cloud,
  Cpu,
  Droplets,
  FlaskConical,
  GitBranch,
  Github,
  Leaf,
  Monitor,
  Shield,
  Thermometer,
  Zap,
} from 'lucide-react';
import Image from 'next/image';
import Link from 'next/link';

import { Accordion, AccordionContent, AccordionItem, AccordionTrigger } from '@/components/ui/accordion';
import { Badge } from '@/components/ui/badge';
import { Button } from '@/components/ui/button';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Separator } from '@/components/ui/separator';
import icon from '@/images/icon.png';
import logo from '@/images/logo.png';
import { MoistureCategory } from '@/lib/prisma/client';

const features = [
  {
    icon: Thermometer,
    title: 'Environmental Monitoring',
    description:
      'Real-time tracking of air temperature, humidity, soil moisture, water temperature, and pH levels across experimental conditions.',
  },
  {
    icon: Zap,
    title: 'Automated Control',
    description:
      'Closed-loop control systems for water pumps and fans, maintaining optimal growing conditions automatically.',
  },
  {
    icon: Cloud,
    title: 'Cloud Integration',
    description:
      'Secure wireless communication with Azure IoT Hub for real-time data visualization and remote device management.',
  },
  {
    icon: Shield,
    title: 'OTA Updates',
    description:
      'Over-the-air firmware updates and device twin management for seamless system maintenance and feature deployment.',
  },
  {
    icon: BarChart3,
    title: 'Data Analytics',
    description:
      'Comprehensive data collection and analysis tools powered by Tinybird for research insights and system performance monitoring.',
  },
  {
    icon: GitBranch,
    title: 'DevOps Integration',
    description:
      'Complete CI/CD pipeline with GitHub Actions, automated testing, and deployment to Azure Container Apps.',
  },
];
const experimentMatrix: { treatment: string; moisture: MoistureCategory; description: string; label: string }[] = [
  { treatment: 'Control', moisture: 'dry', description: 'Baseline untreated seeds in low moisture', label: 'D1' },
  {
    treatment: 'Control',
    moisture: 'medium',
    description: 'Baseline untreated seeds in optimal moisture',
    label: 'M1',
  },
  { treatment: 'Control', moisture: 'wet', description: 'Baseline untreated seeds in high moisture', label: 'W1' },
  { treatment: 'Treatment 1', moisture: 'dry', description: 'Laser treatment 1 in low moisture', label: 'D2' },
  { treatment: 'Treatment 1', moisture: 'medium', description: 'Laser treatment 1 in optimal moisture', label: 'M2' },
  { treatment: 'Treatment 1', moisture: 'wet', description: 'Laser treatment 1 in high moisture', label: 'W2' },
  {
    treatment: '2nd Generation',
    moisture: 'dry',
    description: 'Laser treatment 2nd Generation in low moisture',
    label: 'D3',
  },
  {
    treatment: '2nd Generation',
    moisture: 'medium',
    description: 'Laser treatment 2nd Generation in optimal moisture',
    label: 'M3',
  },
  {
    treatment: '2nd Generation',
    moisture: 'wet',
    description: 'Laser treatment 2nd Generation in high moisture',
    label: 'W3',
  },
];
const stack = [
  { name: 'ESP32', category: 'Hardware', description: 'Micro-controller platform' },
  { name: 'RW612', category: 'Hardware', description: 'NXP Freedom board featuring the RW612 wireless transceiver' },
  { name: 'nRF5x/nRF7x', category: 'Hardware', description: 'Nordic Semiconductor Wi-Fi IC solution' },
  {
    name: 'Zephyr RTOS',
    category: 'Embedded Platform',
    description: 'Open-source real-time operating system for connected devices',
  },
  {
    name: 'ESP-IDF',
    category: 'Embedded Platform',
    description: 'Espressif official IoT development framework for ESP32',
  },
  { name: 'PlatformIO', category: 'Dev Tools', description: 'Embedded development platform' },
  { name: 'Azure IoT', category: 'Cloud', description: 'Device provisioning, messaging & management' },
  { name: 'Neon PostgreSQL', category: 'Database', description: 'Sensor data storage' },
  { name: 'Vercel', category: 'Frontend', description: 'Web dashboard hosting' },
  { name: 'Tinybird', category: 'Analytics', description: 'Real-time data processing' },
  { name: 'GitHub Actions', category: 'DevOps', description: 'CI/CD automation' },
  { name: 'Azure Container Apps', category: 'Cloud', description: 'Containerized services' },
];

export default function Home() {
  return (
    <div className="bg-background min-h-screen">
      {/* Hero section */}
      <section className="to-background relative overflow-hidden bg-gradient-to-b from-green-50 py-20 lg:py-32">
        <div className="container mx-auto px-6">
          <div className="mx-auto max-w-4xl text-center">
            <div className="mb-8 flex justify-center">
              <Image src={logo} alt="Korra Logo" width={300} height={120} className="h-20 w-auto" />
            </div>
            <h1 className="mb-6 text-4xl font-bold tracking-tight text-gray-900 sm:text-6xl">
              ESP32-Based Smart Greenhouse
              <span className="text-green-600"> Monitoring System</span>
            </h1>
            <p className="mb-8 text-xl leading-relaxed text-gray-600">
              An open-source IoT platform for monitoring laser-treated seed growth under controlled soil moisture
              conditions. Built for academic research and educational purposes.
            </p>
            <div className="flex flex-col justify-center gap-4 sm:flex-row">
              <Button size="lg" asChild>
                <Link href="/dashboard/devices">
                  View Dashboard
                  <ArrowRight className="ml-2 h-4 w-4" />
                </Link>
              </Button>
              <Button size="lg" variant="outline" asChild>
                <a href="https://github.com/mburumaxwell/korra" target="_blank" rel="noopener noreferrer">
                  <Github className="mr-2 h-4 w-4" />
                  View on GitHub
                </a>
              </Button>
            </div>
            <div className="mt-8 flex flex-wrap justify-center gap-2">
              <Badge variant="secondary">MSc Dissertation Project</Badge>
              <Badge variant="secondary">Open Source</Badge>
              <Badge variant="secondary">Educational</Badge>
              <Badge variant="secondary">Research</Badge>
            </div>
          </div>
        </div>
      </section>
      {/* Project Overview */}
      <section className="py-20">
        <div className="container mx-auto px-6">
          <div className="mx-auto max-w-4xl">
            <div className="mb-16 text-center">
              <h2 className="mb-4 text-3xl font-bold text-gray-900">Project Overview</h2>
              <p className="text-xl text-gray-600">
                A comprehensive IoT solution demonstrating modern embedded systems, cloud integration, and agricultural
                research methodologies.
              </p>
            </div>

            <div className="mb-16 grid gap-8 md:grid-cols-2">
              <Card>
                <CardHeader>
                  <CardTitle className="flex items-center">
                    <FlaskConical className="mr-2 h-5 w-5 text-green-600" />
                    Research Objectives
                  </CardTitle>
                </CardHeader>
                <CardContent className="space-y-4">
                  <p className="text-sm text-gray-600">
                    Study environmental responses of laser-treated seeds across a 3×3 experimental matrix combining:
                  </p>
                  <ul className="space-y-2 text-sm">
                    <li className="flex items-center">
                      <div className="mr-2 h-2 w-2 rounded-full bg-green-600" />
                      Three laser treatment options (control + 2 treatments)
                    </li>
                    <li className="flex items-center">
                      <div className="mr-2 h-2 w-2 rounded-full bg-green-600" />
                      Three soil moisture levels (dry, medium, wet)
                    </li>
                    <li className="flex items-center">
                      <div className="mr-2 h-2 w-2 rounded-full bg-green-600" />
                      Weekly manual plant height measurements
                    </li>
                  </ul>
                </CardContent>
              </Card>

              <Card>
                <CardHeader>
                  <CardTitle className="flex items-center">
                    <Cpu className="mr-2 h-5 w-5 text-green-600" />
                    Technical Showcase
                  </CardTitle>
                </CardHeader>
                <CardContent className="space-y-4">
                  <p className="text-sm text-gray-600">Demonstrates key embedded systems and IoT concepts:</p>
                  <ul className="space-y-2 text-sm">
                    <li className="flex items-center">
                      <div className="mr-2 h-2 w-2 rounded-full bg-green-600" />
                      Over-the-air (OTA) firmware updates
                    </li>
                    <li className="flex items-center">
                      <div className="mr-2 h-2 w-2 rounded-full bg-green-600" />
                      Device-to-cloud messaging protocols
                    </li>
                    <li className="flex items-center">
                      <div className="mr-2 h-2 w-2 rounded-full bg-green-600" />
                      Polyglot system architecture
                    </li>
                    <li className="flex items-center">
                      <div className="mr-2 h-2 w-2 rounded-full bg-green-600" />
                      Complete DevOps pipeline
                    </li>
                  </ul>
                </CardContent>
              </Card>
            </div>
          </div>
        </div>
      </section>
      {/* Features Section */}
      <section className="bg-gray-50 py-20">
        <div className="container mx-auto px-6">
          <div className="mb-16 text-center">
            <h2 className="mb-4 text-3xl font-bold text-gray-900">System Features</h2>
            <p className="mx-auto max-w-2xl text-xl text-gray-600">
              A modular, scalable platform designed for both educational purposes and practical agricultural research.
            </p>
          </div>

          <div className="grid gap-8 md:grid-cols-2 lg:grid-cols-3">
            {features.map((feature, index) => (
              <Card key={index} className="border-0 shadow-sm transition-shadow hover:shadow-md">
                <CardHeader>
                  <CardTitle className="flex items-center text-lg">
                    <feature.icon className="mr-3 h-6 w-6 text-green-600" />
                    {feature.title}
                  </CardTitle>
                </CardHeader>
                <CardContent>
                  <p className="text-gray-600">{feature.description}</p>
                </CardContent>
              </Card>
            ))}
          </div>
        </div>
      </section>
      {/* Device Architecture */}
      <section className="py-20">
        <div className="container mx-auto px-6">
          <div className="mb-16 text-center">
            <h2 className="mb-4 text-3xl font-bold text-gray-900">Device Architecture</h2>
            <p className="text-xl text-gray-600">
              Two distinct device types working together to create a comprehensive monitoring ecosystem.
            </p>
          </div>

          <div className="mx-auto grid max-w-4xl gap-8 md:grid-cols-2">
            <Card className="border-green-200">
              <CardHeader>
                <CardTitle className="flex items-center text-green-700">
                  <Monitor className="mr-2 h-5 w-5" />
                  Keeper Devices
                </CardTitle>
                <CardDescription>Environmental control and monitoring</CardDescription>
              </CardHeader>
              <CardContent className="space-y-4">
                <p className="text-sm text-gray-600">
                  Manage greenhouse sections and overall environmental conditions.
                </p>
                <div className="space-y-2">
                  <div className="flex items-center text-sm">
                    <Thermometer className="mr-2 h-4 w-4 text-red-500" />
                    Air temperature monitoring
                  </div>
                  <div className="flex items-center text-sm">
                    <Droplets className="mr-2 h-4 w-4 text-blue-500" />
                    Humidity control and measurement
                  </div>
                  <div className="flex items-center text-sm">
                    <Zap className="mr-2 h-4 w-4 text-yellow-500" />
                    Fan control systems
                  </div>
                </div>
              </CardContent>
            </Card>

            <Card className="border-blue-200">
              <CardHeader>
                <CardTitle className="flex items-center text-blue-700">
                  <Leaf className="mr-2 h-5 w-5" />
                  Pot Devices
                </CardTitle>
                <CardDescription>Individual plant monitoring</CardDescription>
              </CardHeader>
              <CardContent className="space-y-4">
                <p className="text-sm text-gray-600">
                  Monitor individual plant pots and their specific growing conditions.
                </p>
                <div className="space-y-2">
                  <div className="flex items-center text-sm">
                    <Droplets className="mr-2 h-4 w-4 text-green-500" />
                    Soil moisture sensing
                  </div>
                  <div className="flex items-center text-sm">
                    <FlaskConical className="mr-2 h-4 w-4 text-purple-500" />
                    pH level monitoring
                  </div>
                  <div className="flex items-center text-sm">
                    <Zap className="mr-2 h-4 w-4 text-yellow-500" />
                    Pump control systems
                  </div>
                </div>
              </CardContent>
            </Card>
          </div>
        </div>
      </section>
      {/* Experiment Matrix */}
      <section className="bg-gray-50 py-20">
        <div className="container mx-auto px-6">
          <div className="mb-16 text-center">
            <h2 className="mb-4 text-3xl font-bold text-gray-900">3×3 Experimental Matrix</h2>
            <p className="text-xl text-gray-600">
              Nine distinct experimental conditions combining laser treatments with soil moisture levels.
            </p>
          </div>

          <div className="mx-auto max-w-6xl">
            <div className="mb-8 grid grid-cols-3 gap-12">
              <div className="text-center font-semibold text-gray-700">Control</div>
              <div className="text-center font-semibold text-gray-700">Treatment A</div>
              <div className="text-center font-semibold text-gray-700">2nd Generation</div>
            </div>

            <div className="grid grid-cols-3 gap-12">
              {experimentMatrix.map((condition, index) => (
                <Card key={index} className="text-center">
                  <CardContent className="p-4">
                    <p>{condition.label}</p>
                    <div className="mt-1">
                      <Badge
                        variant={
                          condition.moisture === 'dry'
                            ? 'outline'
                            : condition.moisture === 'medium'
                              ? 'default'
                              : 'secondary'
                        }
                        className="mb-2"
                      >
                        <span className="capitalize">{condition.moisture}</span> Moisture
                      </Badge>
                    </div>
                    <p className="mt-2 text-xs text-gray-600">{condition.description}</p>
                  </CardContent>
                </Card>
              ))}
            </div>
          </div>
        </div>
      </section>
      {/* Technology Stack */}
      <section className="py-20">
        <div className="container mx-auto px-6">
          <div className="mb-16 text-center">
            <h2 className="mb-4 text-3xl font-bold text-gray-900">Technology Stack</h2>
            <p className="text-xl text-gray-600">
              Built with modern cloud-native technologies and industry best practices.
            </p>
          </div>

          <div className="mx-auto grid max-w-6xl gap-6 md:grid-cols-2 lg:grid-cols-4">
            {stack.map((tech, index) => (
              <Card key={index} className="border-0 text-center shadow-sm">
                <CardContent className="p-4">
                  <Badge variant="outline" className="mb-2">
                    {tech.category}
                  </Badge>
                  <h3 className="mb-1 font-semibold text-gray-900">{tech.name}</h3>
                  <p className="text-xs text-gray-600">{tech.description}</p>
                </CardContent>
              </Card>
            ))}
          </div>
        </div>
      </section>
      {/* FAQ Section */}
      <section className="bg-gray-50 py-20">
        <div className="container mx-auto px-6">
          <div className="mb-16 text-center">
            <h2 className="mb-4 text-3xl font-bold text-gray-900">Frequently Asked Questions</h2>
            <p className="text-xl text-gray-600">Common questions about the Korra greenhouse monitoring system.</p>
          </div>

          <div className="mx-auto max-w-3xl">
            <Accordion type="single" collapsible className="space-y-4">
              <AccordionItem value="item-1">
                <AccordionTrigger>What is the purpose of this project?</AccordionTrigger>
                <AccordionContent>
                  Korra is an MSc dissertation project designed to demonstrate modern embedded systems concepts while
                  conducting agricultural research on laser-treated seed growth. It serves both educational and research
                  purposes, showcasing IoT technologies, cloud integration, and automated control systems.
                </AccordionContent>
              </AccordionItem>

              <AccordionItem value="item-2">
                <AccordionTrigger>Is this system available for commercial use?</AccordionTrigger>
                <AccordionContent>
                  Currently, Korra is an open-source educational project available for free. While there may be
                  commercial considerations in the future, the primary focus is on academic research and teaching
                  embedded systems concepts.
                </AccordionContent>
              </AccordionItem>

              <AccordionItem value="item-3">
                <AccordionTrigger>What makes this different from other greenhouse monitoring systems?</AccordionTrigger>
                <AccordionContent>
                  Korra emphasizes the complete development lifecycle, from embedded firmware to cloud deployment. It
                  demonstrates OTA updates, device twin management, polyglot architectures, and comprehensive DevOps
                  practices - making it ideal for educational purposes and research applications.
                </AccordionContent>
              </AccordionItem>

              <AccordionItem value="item-4">
                <AccordionTrigger>Can I contribute to the project?</AccordionTrigger>
                <AccordionContent>
                  Yes! As an open-source project, contributions are welcome. The codebase is available on GitHub, and we
                  encourage contributions from students, researchers, and hobbyists interested in IoT, agriculture, or
                  embedded systems.
                </AccordionContent>
              </AccordionItem>

              <AccordionItem value="item-5">
                <AccordionTrigger>What sensors and actuators are supported?</AccordionTrigger>
                <AccordionContent>
                  The system supports air temperature and humidity sensors, soil moisture sensors, pH meters, water
                  temperature sensors, water pumps, and ventilation fans. The modular design allows for easy expansion
                  to additional sensor types and control systems.
                </AccordionContent>
              </AccordionItem>

              <AccordionItem value="item-6">
                <AccordionTrigger>How is the data stored and processed?</AccordionTrigger>
                <AccordionContent>
                  Sensor data is transmitted to Azure IoT Hub, stored in Neon PostgreSQL database, and processed using
                  Tinybird for real-time analytics. The web dashboard is hosted on Vercel, providing real-time
                  visualization and device management capabilities.
                </AccordionContent>
              </AccordionItem>
            </Accordion>
          </div>
        </div>
      </section>
      {/* CTA Section */}
      <section className="bg-green-600 py-20">
        <div className="container mx-auto px-6 text-center">
          <div className="mx-auto max-w-3xl">
            <h2 className="mb-4 text-3xl font-bold text-white">Explore the Future of Smart Agriculture</h2>
            <p className="mb-8 text-xl text-green-100">
              Discover how modern IoT technologies can revolutionize greenhouse monitoring and agricultural research.
            </p>
            <div className="flex flex-col justify-center gap-4 sm:flex-row">
              <Button size="lg" variant="secondary" asChild>
                <Link href="/dashboard/devices">
                  View Live Dashboard
                  <ArrowRight className="ml-2 h-4 w-4" />
                </Link>
              </Button>
              <Button
                size="lg"
                variant="outline"
                className="border-white bg-transparent text-white hover:bg-white hover:text-green-600"
                asChild
              >
                <a href="https://github.com/mburumaxwell/korra" target="_blank" rel="noopener noreferrer">
                  <Github className="mr-2 h-4 w-4" />
                  Explore Code
                </a>
              </Button>
            </div>
          </div>
        </div>
      </section>
      {/* Footer */}
      <footer className="bg-gray-900 py-12 text-gray-300">
        <div className="container mx-auto px-6">
          <div className="flex flex-col items-center justify-between md:flex-row">
            <div className="mb-4 flex items-center md:mb-0">
              <Image src={icon} alt="Korra Icon" width={32} height={32} className="mr-2" />
              <span className="text-xl font-bold text-white">Korra</span>
            </div>
            <div className="text-center md:text-right">
              <p className="text-sm">MSc Dissertation Project</p>
              <p className="mt-1 text-xs text-gray-500">ESP32-Based Smart Greenhouse Monitoring System</p>
            </div>
          </div>
          <Separator className="my-8 bg-gray-700" />
          <div className="text-center text-sm text-gray-500">
            <p>© 2025 Korra Project. Open source educational project.</p>
          </div>
        </div>
      </footer>
    </div>
  );
}

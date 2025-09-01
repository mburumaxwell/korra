'use client';

import { BarChart3, Cog, Download, Flower2, Menu, X } from 'lucide-react';
import Image from 'next/image';
import Link from 'next/link';
import { usePathname } from 'next/navigation';
import { useState } from 'react';

import { Button } from '@/components/ui/button';
import avatarImage from '@/images/avatar.png';
import { type Environment } from '@/lib/environment';
import { cn } from '@/lib/utils';
import { Route } from 'next';

const navigationItems = [
  { title: 'Devices', href: '/dashboard/devices', icon: Flower2 },
  { title: 'Firmware', href: '/dashboard/firmware', icon: Download },
  { title: 'Analytics', href: '/dashboard/analytics', icon: BarChart3 },
  { title: 'Settings', href: '/dashboard/settings', icon: Cog },
];

export function Navigation({ environment }: { environment: Environment }) {
  const pathname = usePathname();
  const [isMobileMenuOpen, setIsMobileMenuOpen] = useState(false);

  return (
    <>
      {/* Mobile menu button */}
      <div className="fixed top-4 left-4 z-50 lg:hidden">
        <Button
          variant="outline"
          size="sm"
          onClick={() => setIsMobileMenuOpen(!isMobileMenuOpen)}
          className="bg-background"
        >
          {isMobileMenuOpen ? <X className="h-4 w-4" /> : <Menu className="h-4 w-4" />}
        </Button>
      </div>

      {/* Sidebar */}
      <aside
        className={cn(
          'bg-background fixed top-0 left-0 z-40 h-screen w-64 transform border-r transition-transform duration-200 ease-in-out',
          isMobileMenuOpen ? 'translate-x-0' : '-translate-x-full lg:translate-x-0',
        )}
      >
        <div className="flex h-full flex-col">
          {/* Logo/Header */}
          <div className="flex h-16 items-center border-b px-6">
            <Link href="/" className="flex items-center space-x-2">
              <Image src={avatarImage} alt="" className="h-6 w-6" priority />
              <span className="text-xl font-bold">Korra</span>
            </Link>
          </div>

          {/* Navigation Items */}
          <nav className="flex-1 space-y-1 p-4">
            {navigationItems.map((item) => {
              const Icon = item.icon;
              const isActive = pathname === item.href || (item.href !== '/' && pathname.startsWith(item.href));

              return (
                <Link
                  key={item.href}
                  href={item.href as Route}
                  onClick={() => setIsMobileMenuOpen(false)}
                  className={cn(
                    'flex items-center space-x-3 rounded-lg px-3 py-2 text-sm font-medium transition-colors',
                    isActive
                      ? 'bg-primary text-primary-foreground'
                      : 'text-muted-foreground hover:bg-accent hover:text-accent-foreground',
                  )}
                >
                  <Icon className="h-4 w-4" />
                  <span>{item.title}</span>
                </Link>
              );
            })}
          </nav>

          {/* Footer */}
          <div className="border-t p-4">
            <div className="text-muted-foreground text-xs">
              <p>Korra IoT Manager</p>
              <p className="mt-1">
                <a
                  href={`https://github.com/mburumaxwell/korra/tree/${environment.sha}`}
                  className="underline-offset-4 hover:underline"
                >
                  {environment.branch}@{environment.sha?.slice(0, 7)}
                  {environment.development && <i> **dogfood</i>}
                </a>
              </p>
            </div>
          </div>
        </div>
      </aside>

      {/* Mobile overlay */}
      {isMobileMenuOpen && (
        <div className="fixed inset-0 z-30 bg-black/50 lg:hidden" onClick={() => setIsMobileMenuOpen(false)} />
      )}
    </>
  );
}

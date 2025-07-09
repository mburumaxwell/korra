/**
 * Wraps the Open Graph image component with additional styling and layout properties.
 *
 * @param children - The content to be rendered inside the wrapper.
 * @param props - Additional props to be spread onto the wrapper div element.
 * @returns The wrapped Open Graph image component.
 */
export function OpenGraphImageWrapper({ children, ...props }: React.ComponentPropsWithoutRef<'div'>) {
  return (
    <div
      style={{
        width: '100%',
        height: '100%',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        ...props.style,
      }}
      {...props}
    >
      {children}
    </div>
  );
}
